// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 Yesid Fonseca


#include "AndroidCamera2Subsystem.h"
#include "AndroidCamera2Settings.h"
#include "Stats/Stats.h"
#include "Engine/TextureRenderTarget2D.h"
#include "IMediaClockSink.h"
#include "IMediaModule.h"
#include "IMediaClock.h"

#if PLATFORM_ANDROID
#include "AndroidCamera2Java.h"
#endif

DECLARE_STATS_GROUP(TEXT("AndroidCamera2"), STATGROUP_AndroidCamera2, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Upload Media TickFetch - GameThread (CPU)"), STAT_UploadI420_TickFetch_GT, STATGROUP_AndroidCamera2);
DECLARE_CYCLE_STAT(TEXT("Upload Media TickFetch - RenderThread (GPU)"), STAT_UploadI420_TickFetch_RT, STATGROUP_AndroidCamera2);
DECLARE_FLOAT_COUNTER_STAT_EXTERN(TEXT("1. Upload Media TickFetch - RenderThread spikes >2ms in 1 sec [%]"), STAT_MediaTickFetchGPUSpikesPct_1s, STATGROUP_AndroidCamera2,);
DECLARE_FLOAT_COUNTER_STAT_EXTERN(TEXT("1. Upload Media TickFetch - GameThread spikes >2ms in 1 sec [%]"), STAT_MediaTickFetchCPUSpikesPct_1s, STATGROUP_AndroidCamera2, );
DEFINE_STAT(STAT_MediaTickFetchGPUSpikesPct_1s);
DEFINE_STAT(STAT_MediaTickFetchCPUSpikesPct_1s);

struct FRollingSpikeCounter
{
    double WindowSec;
    int32  NumBuckets;
    uint64 BucketCycles;

    TArray<int32> Frames;
    TArray<int32> Spikes;
    uint64 CurrentBucketId = 0;

    explicit FRollingSpikeCounter(double InWindowSec, int32 InBuckets)
        : WindowSec(InWindowSec)
        , NumBuckets(FMath::Max(1, InBuckets))
        , BucketCycles(InWindowSec / FMath::Max(1.0, (double)InBuckets)/ FPlatformTime::GetSecondsPerCycle64())
    {
        Frames.Init(0, NumBuckets);
        Spikes.Init(0, NumBuckets);
    }

    void AddSample(bool bSpike, int64 NowCycles)
    {
        const uint64 BucketId = (NowCycles / BucketCycles);
        const int32 Idx = (int32)(BucketId % NumBuckets);
        if (BucketId != CurrentBucketId)
        {
            Frames[Idx] = 0;
            Spikes[Idx] = 0;
            CurrentBucketId = BucketId;
        }

        Frames[Idx] += 1;
        if (bSpike) Spikes[Idx] += 1;
    }

    float GetPercent() const
    {
        uint32 F = 0, S = 0;
        for (int32 i = 0; i < NumBuckets; ++i) { F += Frames[i]; S += Spikes[i]; }
        return (F > 0) ? 100.0f * (float(S) / float(F)) : 0.0f;
    }
};

class FAndroidCamera2ThreadSafe
{
public:

    TArray<uint8> YBuffer, UBuffer, VBuffer;
    void* yJavaBuffer = nullptr;
    void* uJavaBuffer = nullptr;
    void* vJavaBuffer = nullptr;
    bool bRenderYRT = false, bRenderURT = false, bRenderVRT = false;
    bool bUpdateYBuffer = false, bUpdateUBuffer = false, bUpdateVBuffer = false;

#if PLATFORM_ANDROID
    TSharedPtr<FAndroidCamera2Java, ESPMode::ThreadSafe> AndroidCamera2Java;
#endif

    int32 Width = 0, Height = 0;
    int64 Timestamp = 0;
    bool bOnRenderQueued{ false };
    void CheckBuffersSize(int32 InWidth, int32 InHeight)
    {
        if (InWidth <= 0 || InHeight <= 0) return;
        if (YBuffer.Num() != InWidth * InHeight)
        {
            Width = InWidth;
            Height = InHeight;
            if(bUpdateYBuffer)
                YBuffer.SetNumUninitialized(Width * Height, EAllowShrinking::No);
			if (bUpdateUBuffer)
                UBuffer.SetNumUninitialized((Width / 2) * (Height / 2), EAllowShrinking::No);
            if(bUpdateVBuffer)
                VBuffer.SetNumUninitialized((Width / 2) * (Height / 2), EAllowShrinking::No);
        }
    }

    FAndroidCamera2ThreadSafe()
        : Width(0)
        , Height(0)
        , Timestamp(0)
    {
#if PLATFORM_ANDROID
        AndroidCamera2Java = MakeShared<FAndroidCamera2Java, ESPMode::ThreadSafe>();
#endif
    }

    void GetLastFrameInfo()
    {
        if (bOnRenderQueued && GetJavaLastFrameTimeStamp() == Timestamp)
            return;

#if PLATFORM_ANDROID
        
        int32 imgWidth = 0;
        int32 imgHeight = 0;

        AndroidCamera2Java->GetLastPreviewFrameInfo(yJavaBuffer, uJavaBuffer, vJavaBuffer, imgWidth, imgHeight, Timestamp); //This call block java side updates on JavaBuffers
        
        CheckBuffersSize(imgWidth, imgHeight);

        if (bUpdateYBuffer && yJavaBuffer)
        {
            const int32 BytesY = Width * Height;
            FMemory::Memcpy(YBuffer.GetData(), yJavaBuffer, BytesY);
        }

        const int32 BytesUV = (Width * Height) / 4;
        if (bUpdateUBuffer && uJavaBuffer)
        {
            FMemory::Memcpy(UBuffer.GetData(), uJavaBuffer, BytesUV);
        }

        if (bUpdateVBuffer && vJavaBuffer)
        {
            FMemory::Memcpy(VBuffer.GetData(), vJavaBuffer, BytesUV);
        }

        if (!bRenderYRT && !bRenderURT && !bRenderVRT)
        {
            AndroidCamera2Java->ReleaseLastPreviewFrameInfo();
        }
#endif
    }

    void UnblockJavaBuffers()
    {
        bOnRenderQueued = false;
#if PLATFORM_ANDROID
       
        AndroidCamera2Java->ReleaseLastPreviewFrameInfo();
#endif
    }

    bool GetInitilizedCamaraState() const
    {
#if PLATFORM_ANDROID
        return AndroidCamera2Java->GetInitilizedCamaraState();
#endif
		return false;
    }

    void ReleaseCamera()
    {
        
#if PLATFORM_ANDROID
        AndroidCamera2Java->Release();
#endif
    }

	int64 GetJavaLastFrameTimeStamp() const 
    { 
#if PLATFORM_ANDROID
        return AndroidCamera2Java->GetLastFrameTimeStamp();
#endif
        return Timestamp; 
    }

    void EnsureRT_G8(UTextureRenderTarget2D* RT, int32 W, int32 H)
    {
        if (!IsValid(RT)) return;
        const bool bFormatOK = (RT->GetFormat() == PF_G8);
        const bool bSizeOK = (RT->SizeX == W && RT->SizeY == H);
        if (!bFormatOK || !bSizeOK)
        {
            RT->InitCustomFormat(W, H, PF_G8, /*bForceLinearGamma*/ false);
        }
    };


    TArray<FString> GetCameraIdList()
    {
        TArray<FString>CameraList = TArray<FString>();

#if PLATFORM_ANDROID
        CameraList = AndroidCamera2Java->GetCameraIdList();      
#endif

        return CameraList;
    }

    bool InitializeCamera(const FString& CameraId, EAndroidCamera2AEMode AEMode, EAndroidCamera2AFMode AFMode, EAndroidCamera2AWBMode AWBMode, EAndroidCamera2ControlMode ControlMode,
        EAndroidCamera2RotationMode RotMode, int32 previewWidth, int32 previewHeight, int32 targetFPS)
    {
#if PLATFORM_ANDROID
        AndroidCamera2Java->Release();
        return AndroidCamera2Java->InitializeCamera(
            CameraId,
            static_cast<uint8>(AEMode),
            static_cast<uint8>(AFMode),
            static_cast<uint8>(AWBMode),
            static_cast<uint8>(ControlMode),
            static_cast<uint8>(RotMode),
            previewWidth,
            previewHeight,
            previewWidth,   //TODO: missing functionality for stillCapure
            previewHeight,  //TODO: missing functionality for stillCapure
            targetFPS
        );
#endif
        return false;
    }

    bool GetIntrinsics(FString CameraId, FAndroidCamera2Intrinsics& Intrinsics)
    {
		Intrinsics = FAndroidCamera2Intrinsics();
        #if PLATFORM_ANDROID
		return AndroidCamera2Java->GetCameraIntrinsincs(CameraId, Intrinsics.FocalLength.X, Intrinsics.FocalLength.Y, Intrinsics.PrincipalPoint.X, Intrinsics.PrincipalPoint.Y, Intrinsics.Skew, Intrinsics.SensorSizePx.X, Intrinsics.SensorSizePx.Y, Intrinsics.FocalLengthMm, Intrinsics.SensorSizeMM.X, Intrinsics.SensorSizeMM.Y, Intrinsics.SensorOrientation);
        #endif
		return false;
    }

    bool GetLensPose(FString CameraId, FAndroidCamera2LensPose& LensPose)
    {
        LensPose = FAndroidCamera2LensPose();
        FQuat4f Orient = FQuat4f::Identity;
        FVector3f Loc = FVector3f::ZeroVector;
        int32 LensPoseReference = 2;
        bool bOk = false;

#if PLATFORM_ANDROID
		bOk = AndroidCamera2Java->GetCameraLensPose(CameraId, Orient.X, Orient.Y, Orient.Z, Orient.W, Loc.X, Loc.Y, Loc.Z, LensPoseReference);
#endif

        if (bOk)
        {
            LensPose.OrientationDeviceCoord = FQuat(Orient);
            LensPose.LocationDeviceCoord = FVector(Loc);
            FVector Xc = LensPose.OrientationDeviceCoord.RotateVector(FVector(1, 0, 0));
			FVector Yc = LensPose.OrientationDeviceCoord.RotateVector(FVector(0, 1, 0));
			FVector Zc = LensPose.OrientationDeviceCoord.RotateVector(FVector(0, 0, 1));
			// From device coord to UE coord
            auto MapToUE = [](const FVector& V) -> FVector
            {
                return FVector(V.Z, V.X, -V.Y);
				};
            FVector Xe = MapToUE(Xc);
            FVector Ye = MapToUE(Yc);
            FVector Ze = MapToUE(Zc);
			LensPose.OrientationUECoord = FRotationMatrix::MakeFromZX(Ze, -Ye).ToQuat();
			LensPose.LocationUECoord = MapToUE(LensPose.LocationDeviceCoord);
            LensPose.LensPoseReference = (EAndroidCamera2LensPoseReference)LensPoseReference;
            return true;
        }

        return false;
    }

};


class FAndroidCamera2ClockSink
    : public IMediaClockSink
{
public:

    FAndroidCamera2ClockSink(UAndroidCamera2Subsystem& InOwner)
        : Owner(InOwner)
    {
    }

    virtual ~FAndroidCamera2ClockSink() {}

public:

    virtual void TickFetch(FTimespan DeltaTime, FTimespan Timecode) override
    {
        Owner.TickFetch(DeltaTime);
    }

private:

    UAndroidCamera2Subsystem& Owner;
};



void UAndroidCamera2Subsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UAndroidCamera2Settings* AC2Settings = GetMutableDefault<UAndroidCamera2Settings>();

    AndroidCamera2 = MakeShared<FAndroidCamera2ThreadSafe, ESPMode::ThreadSafe>();

    y_RT2D = ValidateRenderTarget(AC2Settings->RenderTargetDataYPlane.RenderTarget2D);
    u_RT2D = ValidateRenderTarget(AC2Settings->RenderTargetDataUPlane.RenderTarget2D);
    v_RT2D = ValidateRenderTarget(AC2Settings->RenderTargetDataVPlane.RenderTarget2D);
    AndroidCamera2->bUpdateYBuffer = AC2Settings->RenderTargetDataYPlane.bCaptureBuffer;
    AndroidCamera2->bUpdateUBuffer = AC2Settings->RenderTargetDataUPlane.bCaptureBuffer;
    AndroidCamera2->bUpdateVBuffer = AC2Settings->RenderTargetDataVPlane.bCaptureBuffer;
    AndroidCamera2->bRenderYRT = (y_RT2D != nullptr) && AC2Settings->RenderTargetDataYPlane.bRender;
    AndroidCamera2->bRenderURT = (u_RT2D != nullptr) && AC2Settings->RenderTargetDataUPlane.bRender;
    AndroidCamera2->bRenderVRT = (v_RT2D != nullptr) && AC2Settings->RenderTargetDataVPlane.bRender;

	CameraTimeout = AC2Settings->CameraTimeOut;
}

void UAndroidCamera2Subsystem::Deinitialize()
{
	AndroidCamera2->ReleaseCamera();

}

UTextureRenderTarget2D* UAndroidCamera2Subsystem::ValidateRenderTarget(TSoftObjectPtr<UTextureRenderTarget2D> RenderTarget2D)
{
	UTextureRenderTarget2D* RT = nullptr;
    if (RenderTarget2D.IsNull())
    {
        UE_LOG(LogTemp, Warning, TEXT("UAndroidCamera2Subsystem::ValidateRenderTarget::No RenderTarget2D asignado"));
        return RT;
    }
    if (RenderTarget2D.IsValid())
    {
        RT = RenderTarget2D.Get();
    }
    else
    {
        RT = RenderTarget2D.LoadSynchronous();
        if (!IsValid(RT))
        {
            UE_LOG(LogTemp, Warning, TEXT("UAndroidCamera2Subsystem::ValidateRenderTarget::No se pudo cargar el RenderTarget2D."));
        }        
    }
    return RT;
}

bool UAndroidCamera2Subsystem::GetLuminanceBufferPtr(const uint8*& OutPtr,
    int32& OutWidth,
    int32& OutHeight,
    int64& OutTimestamp) const
{
    if (AndroidCamera2->Width <= 0 || AndroidCamera2->Height <= 0 || AndroidCamera2->YBuffer.Num() == 0)
        return false;

    OutPtr = AndroidCamera2->YBuffer.GetData();
    OutWidth = AndroidCamera2->Width;
    OutHeight = AndroidCamera2->Height;
    OutTimestamp = AndroidCamera2->Timestamp;

    return true;
}

bool UAndroidCamera2Subsystem::GetCbChromaBufferPtr(const uint8*& OutPtr,
    int32& OutWidth,
    int32& OutHeight,
    int64& OutTimestamp) const
{
    if (AndroidCamera2->Width <= 0 || AndroidCamera2->Height <= 0 || AndroidCamera2->YBuffer.Num() == 0)
        return false;

    OutPtr = AndroidCamera2->UBuffer.GetData();
    OutWidth = AndroidCamera2->Width/2;
    OutHeight = AndroidCamera2->Height/2;
    OutTimestamp = AndroidCamera2->Timestamp;

    return true;
}

bool UAndroidCamera2Subsystem::GetCrChromaBufferPtr(const uint8*& OutPtr,
    int32& OutWidth,
    int32& OutHeight,
    int64& OutTimestamp) const
{

    if (AndroidCamera2->Width <= 0 || AndroidCamera2->Height <= 0 || AndroidCamera2->YBuffer.Num() == 0)
        return false;

    OutPtr = AndroidCamera2->VBuffer.GetData();
    OutWidth = AndroidCamera2->Width / 2;
    OutHeight = AndroidCamera2->Height / 2;
    OutTimestamp = AndroidCamera2->Timestamp;

    return true;
}

void UAndroidCamera2Subsystem::SetCameraTimeout(float NewTimeout)
{
    if (NewTimeout <= 0.f) return;
    CameraTimeout = NewTimeout;
}

void UAndroidCamera2Subsystem::PauseCamera()
{
    if (CameraState == EAndroidCamera2State::INITIALIZED)
    {        
        CameraState = EAndroidCamera2State::PAUSED;
	}
}

void UAndroidCamera2Subsystem::ResumeCamera()
{
    if (CameraState == EAndroidCamera2State::PAUSED)
    {
        if (AndroidCamera2->GetInitilizedCamaraState())
        {
            CameraState = EAndroidCamera2State::INITIALIZED;
        }    
    }
}

void UAndroidCamera2Subsystem::StopCamera()
{
    if (AndroidCamera2->GetInitilizedCamaraState())
    {
        AndroidCamera2->ReleaseCamera();
    }	
    CameraState = EAndroidCamera2State::OFF;

    
    if (ClockSink.IsValid())
    {
        IMediaModule* MediaModule = FModuleManager::LoadModulePtr<IMediaModule>("Media");
        if (MediaModule)
        {
            MediaModule->GetClock().RemoveSink(ClockSink.ToSharedRef());
            ClockSink.Reset();
        }
    }
}


UAndroidCamera2Subsystem::UAndroidCamera2Subsystem()
{

}

void UAndroidCamera2Subsystem::TickFetch(FTimespan DeltaTime)
{
    SCOPE_CYCLE_COUNTER(STAT_UploadI420_TickFetch_GT);
    const uint64 T0 = FPlatformTime::Cycles64();
    switch (CameraState)
    {
	case EAndroidCamera2State::OFF:
		break;

    case EAndroidCamera2State::WAITING_INIT: // Waiting for Initialization
        CameraTimeLeftAfterInitialization -= DeltaTime.GetTotalSeconds();
        if (AndroidCamera2->GetInitilizedCamaraState())
        {
			CameraState = EAndroidCamera2State::INITIALIZED; // Initialized
        }
        else if(CameraTimeLeftAfterInitialization<CameraTimeout)
        {
			CameraState = EAndroidCamera2State::FAIL_INIT; // Fail to initialize
        }
        break;
	case EAndroidCamera2State::INITIALIZED: // Initialized
       
        AndroidCamera2->GetLastFrameInfo();
        UpdateRenderTextures();
       
        break;

    default:
        break;
    }

	const uint64 T1 = FPlatformTime::Cycles64();

    static FRollingSpikeCounter GT_W1s(1.0, 10);   // 10 buckets de 100 ms

    const bool bSpike =  FPlatformTime::ToMilliseconds64(T1 - T0) > 2.0f;

    GT_W1s.AddSample(bSpike, T0);
    SET_FLOAT_STAT(STAT_MediaTickFetchCPUSpikesPct_1s, GT_W1s.GetPercent());
    
}

TArray<FString> UAndroidCamera2Subsystem::GetCameraIdList()
{
    return AndroidCamera2->GetCameraIdList();
}



void UAndroidCamera2Subsystem::UpdateRenderTextures()
{
    check(IsInGameThread());

    if (AndroidCamera2->bOnRenderQueued)
        return;

    const bool bDoY = (IsValid(y_RT2D) && AndroidCamera2->bRenderYRT && AndroidCamera2->yJavaBuffer && AndroidCamera2->Width > 0 && AndroidCamera2->Height > 0);
    const bool bDoU = (IsValid(u_RT2D) && AndroidCamera2->bRenderURT && AndroidCamera2->uJavaBuffer && AndroidCamera2->Width > 0 && AndroidCamera2->Height > 0);
    const bool bDoV = (IsValid(v_RT2D) && AndroidCamera2->bRenderVRT && AndroidCamera2->vJavaBuffer && AndroidCamera2->Width > 0 && AndroidCamera2->Height > 0);

    if (bDoY) { AndroidCamera2->EnsureRT_G8(y_RT2D, AndroidCamera2->Width, AndroidCamera2->Height); }
    if (bDoU) { AndroidCamera2->EnsureRT_G8(u_RT2D, AndroidCamera2->Width / 2, AndroidCamera2->Height / 2); }
    if (bDoV) { AndroidCamera2->EnsureRT_G8(v_RT2D, AndroidCamera2->Width / 2, AndroidCamera2->Height / 2); }


    FTextureRenderTargetResource* RTResY = bDoY ? y_RT2D->GameThread_GetRenderTargetResource() : nullptr;
    FTextureRenderTargetResource* RTResU = bDoU ? u_RT2D->GameThread_GetRenderTargetResource() : nullptr;
    FTextureRenderTargetResource* RTResV = bDoV ? v_RT2D->GameThread_GetRenderTargetResource() : nullptr;

    if (RTResY == nullptr && RTResU == nullptr && RTResV == nullptr)
    {
        AndroidCamera2->UnblockJavaBuffers();
    }

    const uint8* YPtr = static_cast<const uint8*>(AndroidCamera2->yJavaBuffer);
    const uint8* UPtr = static_cast<const uint8*>(AndroidCamera2->uJavaBuffer);
    const uint8* VPtr = static_cast<const uint8*>(AndroidCamera2->vJavaBuffer);

    AndroidCamera2->bOnRenderQueued = true;
    ENQUEUE_RENDER_COMMAND(UploadI420_All)(
        [AndroidCam2 = AndroidCamera2, RTResY, RTResU, RTResV, YPtr, UPtr, VPtr, W = AndroidCamera2->Width, H = AndroidCamera2->Height](FRHICommandListImmediate& RHICmd)
        {
            SCOPE_CYCLE_COUNTER(STAT_UploadI420_TickFetch_RT);

            const uint64 T0 = FPlatformTime::Cycles64();

            if (AndroidCam2->yJavaBuffer)
            {
                UAndroidCamera2Subsystem::UpdatePlaneTexture_RenderThread(RHICmd, RTResY, YPtr, W, H);
            }
            
            if (AndroidCam2->uJavaBuffer)
            {
                UAndroidCamera2Subsystem::UpdatePlaneTexture_RenderThread(RHICmd, RTResU, UPtr, W/ 2, H/ 2);
            }
            if (AndroidCam2->vJavaBuffer)
            {
                UAndroidCamera2Subsystem::UpdatePlaneTexture_RenderThread(RHICmd, RTResV, VPtr, W/ 2, H/ 2);
            }

            AndroidCam2->UnblockJavaBuffers();

            const uint64 T1 = FPlatformTime::Cycles64();
            
            static FRollingSpikeCounter RT_W1s(1.0, 10);   // 10 buckets de 100 ms

            const bool bSpike = FPlatformTime::ToMilliseconds64(T1 - T0) >2.0f;

            RT_W1s.AddSample(bSpike, T0);
            SET_FLOAT_STAT(STAT_MediaTickFetchGPUSpikesPct_1s, RT_W1s.GetPercent());
        }
        );

}

void UAndroidCamera2Subsystem::UpdatePlaneTexture_RenderThread(FRHICommandListImmediate& RHICmd, FTextureRenderTargetResource* RTRes, const uint8* Src, int32 W, int32 H)
{
    if (!RTRes || !Src) return;
    FRHITexture* RHITexture = RTRes->GetRenderTargetTexture();
    if (!RHITexture) return;
    FRHITexture2D* RHITexture2D = RHITexture->GetTexture2D();
    if (!RHITexture2D) return;
    const FUpdateTextureRegion2D Region(0, 0, 0, 0, W, H);
    RHICmd.UpdateTexture2D(RHITexture2D, /*Mip*/0, Region, (uint32)W, Src);
}

bool UAndroidCamera2Subsystem::InitializeCamera(const FString& CameraId, EAndroidCamera2AEMode AEMode, EAndroidCamera2AFMode AFMode, EAndroidCamera2AWBMode AWBMode, EAndroidCamera2ControlMode ControlMode,
    EAndroidCamera2RotationMode RotMode, int32 previewWidth, int32 previewHeight, int32 targetFPS)
{
    
    CameraState = AndroidCamera2->InitializeCamera(
        CameraId,AEMode,AFMode,AWBMode,ControlMode,RotMode, previewWidth, previewHeight,  targetFPS) ? EAndroidCamera2State::INITIALIZED : EAndroidCamera2State::FAIL_INIT; // Waiting for Initialization
    CameraTimeLeftAfterInitialization = CameraTimeout;


    if (CameraState == EAndroidCamera2State::INITIALIZED)
    {
        ClockSink = MakeShared<FAndroidCamera2ClockSink, ESPMode::ThreadSafe>(*this);
        IMediaModule* MediaModule = FModuleManager::LoadModulePtr<IMediaModule>("Media");
        MediaModule->GetClock().AddSink(ClockSink.ToSharedRef());
    }

    return CameraState == EAndroidCamera2State::INITIALIZED;
}

bool UAndroidCamera2Subsystem::GetCameraIntrinsics(FString CameraId, FAndroidCamera2Intrinsics& Intrinsics)
{
	return AndroidCamera2->GetIntrinsics(CameraId, Intrinsics);
}

bool UAndroidCamera2Subsystem::GetCameraLensPose(FString CameraId, FAndroidCamera2LensPose& LensPose)
{
    return AndroidCamera2->GetLensPose(CameraId, LensPose);
}
