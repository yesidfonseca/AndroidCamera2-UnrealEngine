#include "AndroidCamera2BlueprintLibrary.h"

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

bool UAndroidCamera2BlueprintLibrary::InitializeCamera(const FString& CameraId, EAndroidCamera2AEMode AEMode, EAndroidCamera2AFMode AFMode, EAndroidCamera2AWBMode AWBMode, EAndroidCamera2ControlMode ControlMode)
{
    if (IsValidAC2J())
    {
#if PLATFORM_ANDROID
        return AndroidCamera2Java->InitializeCamera(
            CameraId,
            static_cast<uint8>(AEMode),
            static_cast<uint8>(AFMode),
            static_cast<uint8>(AWBMode),
            static_cast<uint8>(ControlMode)
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
