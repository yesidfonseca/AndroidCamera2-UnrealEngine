// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 Yesid Fonseca


#include "AndroidCamera2Subsystem.h"
#include "AndroidCamera2Settings.h"
#include "Engine/TextureRenderTarget2D.h"

#if PLATFORM_ANDROID
#include "AndroidCamera2Java.h"
#endif

void UAndroidCamera2Subsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UAndroidCamera2Settings* AC2Settings = GetMutableDefault<UAndroidCamera2Settings>();
	bAutoUpdateRenderTargets = AC2Settings->bAutoUpdateRenderTargets;

#if PLATFORM_ANDROID
    IsValidAC2J();
#endif

	y_RT2D = ValidateRenderTarget(AC2Settings->RenderTargetDataYPlane.RenderTarget2D);
	u_RT2D = ValidateRenderTarget(AC2Settings->RenderTargetDataUPlane.RenderTarget2D);
	v_RT2D = ValidateRenderTarget(AC2Settings->RenderTargetDataVPlane.RenderTarget2D);
	bUpdateYBuffer = AC2Settings->RenderTargetDataYPlane.bCaptureBuffer;
	bUpdateUBuffer = AC2Settings->RenderTargetDataUPlane.bCaptureBuffer;
	bUpdateVBuffer = AC2Settings->RenderTargetDataVPlane.bCaptureBuffer;
	bRenderYRT = (y_RT2D != nullptr) && AC2Settings->RenderTargetDataYPlane.bRender && bUpdateYBuffer;
	bRenderURT = (u_RT2D != nullptr) && AC2Settings->RenderTargetDataUPlane.bRender && bUpdateUBuffer;
	bRenderVRT = (v_RT2D != nullptr) && AC2Settings->RenderTargetDataVPlane.bRender && bUpdateVBuffer;

	CameraTimeout = AC2Settings->CameraTimeOut;
}

void UAndroidCamera2Subsystem::Deinitialize()
{
#if PLATFORM_ANDROID
    if (IsValidAC2J())
    {
		AndroidCamera2Java->Release();
    }
#endif
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
            UE_LOG(LogTemp, Error, TEXT("UAndroidCamera2Subsystem::ValidateRenderTarget::No se pudo cargar el RenderTarget2D."));
        }        
    }
    return RT;
}

bool UAndroidCamera2Subsystem::GetLuminanceBufferPtr(const uint8*& OutPtr,
    int32& OutWidth,
    int32& OutHeight,
    int64& OutTimestamp) const
{
    check(IsInGameThread()); 
    if (CurrentWidth <= 0 || CurrentHeight <= 0 || YBuffer.Num() == 0)
        return false;

    OutPtr = YBuffer.GetData();  
    OutWidth = CurrentWidth;
    OutHeight = CurrentHeight;
    OutTimestamp = LastFrameTimestamp; 

    return true;
}

bool UAndroidCamera2Subsystem::GetCbChromaBufferPtr(const uint8*& OutPtr,
    int32& OutWidth,
    int32& OutHeight,
    int64& OutTimestamp) const
{
    check(IsInGameThread());

    if (CurrentWidth <= 0 || CurrentHeight <= 0 || YBuffer.Num() == 0)
        return false;

    OutPtr = UBuffer.GetData();
    OutWidth = CurrentWidth/2;
    OutHeight = CurrentHeight/2;
    OutTimestamp = LastFrameTimestamp;

    return true;
}

bool UAndroidCamera2Subsystem::GetCrChromaBufferPtr(const uint8*& OutPtr,
    int32& OutWidth,
    int32& OutHeight,
    int64& OutTimestamp) const
{
    check(IsInGameThread());

    if (CurrentWidth <= 0 || CurrentHeight <= 0 || YBuffer.Num() == 0)
        return false;

    OutPtr = VBuffer.GetData();
    OutWidth = CurrentWidth / 2;
    OutHeight = CurrentHeight / 2;
    OutTimestamp = LastFrameTimestamp;

    return true;
}

void UAndroidCamera2Subsystem::SetCameraTimeout(float NewTimeout)
{
    if (NewTimeout <= 0.f) return;
    CameraTimeout = NewTimeout;
}

void UAndroidCamera2Subsystem::PauseCamera()
{
    if (CameraState == EAndroidCamera2State::INITIALIZED && bAutoUpdateRenderTargets)
    {        
        CameraState = EAndroidCamera2State::PAUSED;
	}
}

void UAndroidCamera2Subsystem::ResumeCamera()
{
    if (CameraState == EAndroidCamera2State::PAUSED && bAutoUpdateRenderTargets)
    {
#if PLATFORM_ANDROID
        if (AndroidCamera2Java->GetInitilizedCamaraState())
        {
            CameraState = EAndroidCamera2State::INITIALIZED;
        }
#endif        
    }
}

void UAndroidCamera2Subsystem::StopCamera()
{
#if PLATFORM_ANDROID
    if (AndroidCamera2Java->GetInitilizedCamaraState())
    {
        AndroidCamera2Java->Release();
    }	
#endif
    CameraState = EAndroidCamera2State::OFF;
}


void UAndroidCamera2Subsystem::Tick(float DeltaSeconds)
{
    
    switch (CameraState)
    {
	case EAndroidCamera2State::OFF:
		break;
#if PLATFORM_ANDROID
    case EAndroidCamera2State::WAITING_INIT: // Waiting for Initialization
		CameraTimeLeftAfterInitialization -= DeltaSeconds;
        if (AndroidCamera2Java->GetInitilizedCamaraState())
        {
			CameraState = EAndroidCamera2State::INITIALIZED; // Initialized
        }
        else if(CameraTimeLeftAfterInitialization<CameraTimeout)
        {
			CameraState = EAndroidCamera2State::FAIL_INIT; // Fail to initialize
        }
        break;
	case EAndroidCamera2State::INITIALIZED: // Initialized
        if (bAutoUpdateRenderTargets )
        {
            if (LastFrameTimestamp != AndroidCamera2Java->GetLastFrameTimeStamp())
            {
                GetLastFrameInfo();
                UpdateRenderTextures();                
            }            
        }
        break;
#endif
    default:
        break;
    }
    
    
}

bool UAndroidCamera2Subsystem::IsValidAC2J()
{
    bool bIsValid = false;

#if PLATFORM_ANDROID
    if (AndroidCamera2Java.IsValid())
    {
        bIsValid = true;
    }
    else
    {
        AndroidCamera2Java = MakeShared<FAndroidCamera2Java, ESPMode::ThreadSafe>();
    }

    bIsValid = AndroidCamera2Java.IsValid();

#endif

    return bIsValid;
}

TArray<FString> UAndroidCamera2Subsystem::GetCameraIdList()
{
    TArray<FString>CameraList = TArray<FString>();

#if PLATFORM_ANDROID
    if (IsValidAC2J())
    {
        CameraList = AndroidCamera2Java->GetCameraIdList();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UAndroidCamera2Subsystem::GetCameraIdList: AndroidCamera2Java is not valid!"));
    }
#endif

    return CameraList;
}

void UAndroidCamera2Subsystem::GetLastFrameInfo()
{
#if PLATFORM_ANDROID
    if (IsValidAC2J())
    {
        void* yBufferTemp = nullptr;
        void* uBufferTemp = nullptr;
        void* vBufferTemp = nullptr;
        int32 imgWidth = 0;
        int32 imgHeight = 0;

        AndroidCamera2Java->GetLastPreviewFrameInfo(yBufferTemp, uBufferTemp, vBufferTemp, imgWidth, imgHeight, LastFrameTimestamp);

		CurrentWidth = imgWidth;
		CurrentHeight = imgHeight;
        
        if (bUpdateYBuffer)
        {
            const int32 BytesY = imgWidth * imgHeight; 
            if(YBuffer.Num() != BytesY)
            {
                YBuffer.SetNumUninitialized(BytesY);
			}

            
            
            FMemory::Memcpy(YBuffer.GetData(), yBufferTemp, BytesY);
        }

        const int32 BytesUV = (imgWidth * imgHeight)/4; 
        if (bUpdateUBuffer)
        {
            if (UBuffer.Num() != BytesUV)
            {
                UBuffer.SetNumUninitialized(BytesUV);
            }

            FMemory::Memcpy(UBuffer.GetData(), uBufferTemp, BytesUV);
        }

        if (bUpdateVBuffer)
        {
            if (VBuffer.Num() != BytesUV)
            {
                VBuffer.SetNumUninitialized(BytesUV);
            }

            FMemory::Memcpy(VBuffer.GetData(), vBufferTemp, BytesUV);
        }

        AndroidCamera2Java->ReleaseLastPreviewFrameInfo();

    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UAndroidCamera2Subsystem::GetLastFrameInfo: AndroidCamera2Java is not valid!"));
    }
#endif
}

void UAndroidCamera2Subsystem::UpdateRenderTextures()
{
    check(IsInGameThread());

    const bool bDoY = (IsValid(y_RT2D) && bRenderYRT && CurrentWidth > 0 && CurrentHeight > 0);
    const bool bDoU = (IsValid(u_RT2D) && bRenderURT && CurrentWidth > 0 && CurrentHeight > 0);
    const bool bDoV = (IsValid(v_RT2D) && bRenderVRT && CurrentWidth > 0 && CurrentHeight > 0);

    if (!(bDoY || bDoU || bDoV)) return;

    auto EnsureRT_G8 = [](UTextureRenderTarget2D* RT, int32 W, int32 H)
        {
            if (!IsValid(RT)) return;
            const bool bFormatOK = (RT->GetFormat() == PF_G8);
            const bool bSizeOK = (RT->SizeX == W && RT->SizeY == H);
            if (!bFormatOK || !bSizeOK)
            {
                RT->InitCustomFormat(W, H, PF_G8, /*bForceLinearGamma*/ false);
            }
        };

    if (bDoY) { EnsureRT_G8(y_RT2D, CurrentWidth, CurrentHeight); }
    if (bDoU) { EnsureRT_G8(u_RT2D, CurrentWidth/2, CurrentHeight/2); }
    if (bDoV) { EnsureRT_G8(v_RT2D, CurrentWidth/2, CurrentHeight/2); }
    
    
    // 3) Recursos RHI (desde Game Thread)
    FTextureRenderTargetResource* RTResY = bDoY ? y_RT2D->GameThread_GetRenderTargetResource() : nullptr;
    FTextureRenderTargetResource* RTResU = bDoU ? u_RT2D->GameThread_GetRenderTargetResource() : nullptr;
    FTextureRenderTargetResource* RTResV = bDoV ? v_RT2D->GameThread_GetRenderTargetResource() : nullptr;


    // 4) Un único ENQUEUE para subir todos los planos disponibles
    ENQUEUE_RENDER_COMMAND(UploadI420_All)(
        [RTResY, RTResU, RTResV,
        StagingY = TArray<uint8>(YBuffer),
        StagingU = TArray<uint8>(UBuffer),
        StagingV = TArray<uint8>(VBuffer),
        W = CurrentWidth, H = CurrentHeight](FRHICommandListImmediate& RHICmd)
        {
            auto UploadPlane = [&RHICmd](FTextureRenderTargetResource* RTRes, const uint8* Src, int32 W, int32 H)
                {
                    if (!RTRes || !Src) return;
                    FRHITexture* RHITexture = RTRes->GetRenderTargetTexture();
                    if (!RHITexture) return;
                    FRHITexture2D* RHITexture2D = RHITexture->GetTexture2D();
                    if (!RHITexture2D) return;

                    uint32 DestStride = 0;
                    void* DestPtr = RHILockTexture2D(RHITexture2D, /*Mip*/0, RLM_WriteOnly, DestStride, /*bLockWithinMiptail*/ false);
                    if (!DestPtr) return;

                    uint8* Dst = static_cast<uint8*>(DestPtr);
                    const uint32 RowBytes = static_cast<uint32>(W); // 1 byte/px

                    for (int32 y = 0; y < H; ++y)
                    {
                        FMemory::Memcpy(Dst + y * DestStride, Src + y * RowBytes, RowBytes);
                    }

                    RHICmd.UnlockTexture2D(RHITexture2D, /*Mip*/0, /*bLockWithinMiptail*/ false, /*bFlushRHIThread*/ true);
                };

            if (!StagingY.IsEmpty())
            {
                UploadPlane(RTResY, StagingY.GetData(), W, H);
            }
            if (!StagingU.IsEmpty())
            {
                UploadPlane(RTResU, StagingU.GetData(), W/2, H/2);
            }
            if (!StagingV.IsEmpty())
            {
                UploadPlane(RTResV, StagingV.GetData(), W/2, H/2);
            }
        }
        );
}

bool UAndroidCamera2Subsystem::InitializeCamera(const FString& CameraId, EAndroidCamera2AEMode AEMode, EAndroidCamera2AFMode AFMode, EAndroidCamera2AWBMode AWBMode, EAndroidCamera2ControlMode ControlMode,
    EAndroidCamera2RotationMode RotMode, int32 previewWidth, int32 previewHeight, int32 stillCaptureWidth, int32 stillCaptureHeight, int32 targetFPS)
{

#if PLATFORM_ANDROID
    if (IsValidAC2J())
    {
		AndroidCamera2Java->Release(); 
		CameraState = AndroidCamera2Java->InitializeCamera(
            CameraId,
            static_cast<uint8>(AEMode),
            static_cast<uint8>(AFMode),
            static_cast<uint8>(AWBMode),
            static_cast<uint8>(ControlMode),
            static_cast<uint8>(RotMode),
            previewWidth,
            previewHeight,
            stillCaptureWidth,
            stillCaptureHeight,
            targetFPS
        )? EAndroidCamera2State::INITIALIZED : EAndroidCamera2State::FAIL_INIT; // Waiting for Initialization
		CameraTimeLeftAfterInitialization = CameraTimeout;
		return CameraState == EAndroidCamera2State::INITIALIZED;

    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UAndroidCamera2Subsystem::InitializeCamera: AndroidCamera2Java is not valid!"));
    }
#endif
    return false;
}