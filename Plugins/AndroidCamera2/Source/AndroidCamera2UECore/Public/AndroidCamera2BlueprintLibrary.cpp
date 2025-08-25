#include "AndroidCamera2BlueprintLibrary.h"

#include "Engine/TextureRenderTarget2D.h"
#include "RHI.h"
#include "RHIResources.h"
#include "RHICommandList.h"
#include "RenderingThread.h"
#include "RenderResource.h"
#include "PixelFormat.h"

#if PLATFORM_ANDROID
#include "AndroidCamera2Java.h"

static TSharedPtr<FAndroidCamera2Java, ESPMode::ThreadSafe> AndroidCamera2Java;
#endif

TArray<FString> UAndroidCamera2BlueprintLibrary::GetCameraIdList()
{
    TArray<FString>CameraList = TArray<FString>();


    if (IsValidAC2J())
    {
#if PLATFORM_ANDROID
        CameraList = AndroidCamera2Java->GetCameraIdList();
#endif
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UAndroidCamera2BlueprintLibrary::GetCameraIdList: AndroidCamera2Java is not valid!"));
	}


    return CameraList;
}

bool UAndroidCamera2BlueprintLibrary::InitializeCamera(const FString& CameraId, EAndroidCamera2AEMode AEMode, EAndroidCamera2AFMode AFMode, EAndroidCamera2AWBMode AWBMode, EAndroidCamera2ControlMode ControlMode,
    EAndroidCamera2RotationMode RotMode, int32 previewWidth, int32 previewHeight, int32 stillCaptureWidth, int32 stillCaptureHeight, int32 targetFPS)
{
    if (IsValidAC2J())
    {
#if PLATFORM_ANDROID
        return AndroidCamera2Java->InitializeCamera(
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
        );
#endif
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UAndroidCamera2BlueprintLibrary::InitializeCamera: AndroidCamera2Java is not valid!"));
    }

    return false;
}

bool UAndroidCamera2BlueprintLibrary::TakePhoto()
{
    if (IsValidAC2J())
    {
#if PLATFORM_ANDROID
        return AndroidCamera2Java->TakePhoto();
#endif
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UAndroidCamera2BlueprintLibrary::TakePhoto: AndroidCamera2Java is not valid!"));
    }

    return false;
}

bool UAndroidCamera2BlueprintLibrary::GetLastCapturedImage(TArray<uint8>& OutJpegBytes)
{
    return false;
}

bool UAndroidCamera2BlueprintLibrary::SaveResult(FString& OutAbsolutePath)
{
    if (IsValidAC2J())
    {
#if PLATFORM_ANDROID
        return AndroidCamera2Java->SaveResult(OutAbsolutePath);
#endif
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UAndroidCamera2BlueprintLibrary::SaveResult: AndroidCamera2Java is not valid!"));
    }

    return false;
}

void UAndroidCamera2BlueprintLibrary::Release()
{
    // No-op for non-Android platforms
}

void UAndroidCamera2BlueprintLibrary::GetLastFrameInfo(UTextureRenderTarget2D* RT)
{
    if (IsValidAC2J())
    {
        void* Buffer = nullptr;
        int32 imgWidth = 0;
        int32 imgHeight = 0;
#if PLATFORM_ANDROID
       AndroidCamera2Java->GetLastPreviewFrameInfo(Buffer,imgWidth,imgHeight);
#endif
       UpdateYPlaneIntoRT(RT, Buffer, imgWidth, imgHeight);
#if PLATFORM_ANDROID
       AndroidCamera2Java->ReleaseLastPreviewFrameInfo();
#endif
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UAndroidCamera2BlueprintLibrary::SaveResult: AndroidCamera2Java is not valid!"));
    }
}

bool UAndroidCamera2BlueprintLibrary::IsValidAC2J()
{
	bool bIsValid = false;

#if PLATFORM_ANDROID
    if(AndroidCamera2Java.IsValid())
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

// Llama esto en Game Thread
// Buffer: puntero a tu plano Y
// SrcWidth/SrcHeight: dimensiones en píxeles del Y-plane
// SrcStrideBytes: stride en bytes de cada fila del Y-plane (de Camera2 suele ser >= SrcWidth)
void UAndroidCamera2BlueprintLibrary::UpdateYPlaneIntoRT(UTextureRenderTarget2D* RT, const void* Buffer, int32 SrcWidth, int32 SrcHeight)
{
    check(RT);
    check(IsInGameThread());
    if (!Buffer || SrcWidth <= 0 || SrcHeight <= 0)
    {
        return;
    }

    // 1) Validar formato/tamaño (ideal para Y-plane: R8)
    const bool bFormatOK = (RT->GetFormat() == PF_G8); // 1 canal de 8 bits
    const bool bSizeOK = (RT->SizeX == SrcWidth && RT->SizeY == SrcHeight);

    if (!bFormatOK || !bSizeOK)
    {
        // Recrea el RT con el tamaño y formato correctos.
        // Nota: InitCustomFormat recrea el recurso y es seguro en Game Thread.
        RT->InitCustomFormat(
            SrcWidth,
            SrcHeight,
            PF_G8,          // un canal (Y)
            false           // bForceLinearGamma: false (no sRGB)
        );
        // Si prefieres evitar recreaciones repetidas, puedes cachear el tamaño/format y sólo tocarlo cuando cambie.
    }

    // 2) Copia a staging buffer para asegurar la validez del puntero hasta que ejecute el render thread
    TArray<uint8> Staging;
    Staging.SetNumUninitialized(SrcWidth * SrcHeight);

    {
        const uint8* RESTRICT Src = static_cast<const uint8*>(Buffer);
        uint8* RESTRICT Dst = Staging.GetData();
        const int32 RowCopyBytes = SrcWidth; // 1 byte por píxel (R8)

        for (int32 y = 0; y < SrcHeight; ++y)
        {
            FMemory::Memcpy(Dst + y * RowCopyBytes, Src + y * SrcWidth, RowCopyBytes);
        }
    }

    // 3) Obtener el recurso del RT (desde Game Thread)
    FTextureRenderTargetResource* RTRes = RT->GameThread_GetRenderTargetResource();
    if (!RTRes)
    {
        return;
    }

    // 4) Subir al GPU en el Render Thread
    ENQUEUE_RENDER_COMMAND(CopyYPlaneToRT)(
        [RTRes, Staging = MoveTemp(Staging), W = SrcWidth, H = SrcHeight](FRHICommandListImmediate& RHICmdList)
        {
            // Obtener la textura RHI subyacente
            FRHITexture* RHITexture = RTRes->GetRenderTargetTexture();
            if (!RHITexture)
            {
                return;
            }

            FRHITexture2D* RHITexture2D = RHITexture->GetTexture2D();
            if (!RHITexture2D)
            {
                return;
            }

            // Bloquear mip 0 para escritura y copiar fila a fila
            uint32 DestStrideBytes = 0;
            void* DestPtr = RHILockTexture2D(
                RHITexture2D,
                /*MipIndex*/ 0,
                RLM_WriteOnly,
                DestStrideBytes,
                /*bLockWithinMiptail*/ false // usa el mismo valor al desbloquear
            );
            if (!DestPtr)
            {
                return;
            }

            const uint8* RESTRICT Src = Staging.GetData();         // packed W×H
            uint8* RESTRICT Dst = static_cast<uint8*>(DestPtr);
            const uint32 RowCopyBytes = static_cast<uint32>(W);     // 1 byte/pixel

            // Ojo: DestStrideBytes puede ser >= RowCopyBytes
            for (int32 y = 0; y < H; ++y)
            {
                FMemory::Memcpy(Dst + y * DestStrideBytes, Src + y * RowCopyBytes, RowCopyBytes);
            }

            RHIUnlockTexture2D(RHITexture2D, /*MipIndex*/ 0, false, true);
        }
        );
}
