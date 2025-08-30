#include "AndroidCamera2BlueprintLibrary.h"
#include "Kismet/GameplayStatics.h" 
#include "AndroidCamera2Subsystem.h"

#if PLATFORM_ANDROID
#include "AndroidCamera2Java.h"

static TSharedPtr<FAndroidCamera2Java, ESPMode::ThreadSafe> AndroidCamera2Java;
#endif



bool UAndroidCamera2BlueprintLibrary::InitializeCamera(const FString& CameraId, EAndroidCamera2AEMode AEMode, EAndroidCamera2AFMode AFMode, EAndroidCamera2AWBMode AWBMode, EAndroidCamera2ControlMode ControlMode,
    EAndroidCamera2RotationMode RotMode, int32 previewWidth, int32 previewHeight, int32 stillCaptureWidth, int32 stillCaptureHeight, int32 targetFPS)
{
    
    if (UGameInstance* GI = UGameplayStatics::GetGameInstance(GWorld))
    {
        if (auto* Cam2 = GI->GetSubsystem<UAndroidCamera2Subsystem>())
        {
			return Cam2->InitializeCamera(CameraId, AEMode, AFMode, AWBMode, ControlMode, RotMode, previewWidth, previewHeight, stillCaptureWidth, stillCaptureHeight, targetFPS);
        }
    }
    return false;
}

bool UAndroidCamera2BlueprintLibrary::TakePhoto()
{
#if PLATFORM_ANDROID
        return AndroidCamera2Java->TakePhoto();
#endif
   

    return false;
}

bool UAndroidCamera2BlueprintLibrary::GetLastCapturedImage(TArray<uint8>& OutJpegBytes)
{
    return false;
}

bool UAndroidCamera2BlueprintLibrary::SaveResult(FString& OutAbsolutePath)
{
    

    return false;
}

TArray<FString> UAndroidCamera2BlueprintLibrary::GetCameraIdList()
{
    TArray<FString>CameraList = TArray<FString>();

    if (UGameInstance* GI = UGameplayStatics::GetGameInstance(GWorld))
    {
        if (auto* Cam2 = GI->GetSubsystem<UAndroidCamera2Subsystem>())
        {
            CameraList = Cam2->GetCameraIdList();
        }
    }
    return CameraList;
}

