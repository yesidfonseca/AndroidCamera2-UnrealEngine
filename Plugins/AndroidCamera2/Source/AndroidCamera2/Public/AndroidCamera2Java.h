#pragma once

#include "CoreMinimal.h"
#include "Android/AndroidJava.h"

// Wrapper for com/FonseCode/Camera2UE.java.
class FAndroidCamera2Java : public FJavaClassObject
{

public:
	FAndroidCamera2Java();
	virtual ~FAndroidCamera2Java();
	TArray<FString> GetCameraIdList();
	bool InitializeCamera(const FString& CameraId, uint8 AEMode, uint8 AFMode, uint8 AWBMode, uint8 ControMode);
    bool TakePhoto() ;
    bool GetLastCapturedImage(TArray<uint8>& OutJpeg) const;
    bool SaveResult(FString& OutAbsolutePath);
    void Release() const;

private:
	static FName GetClassName();

	FJavaClassMethod GetCameraIdListMethod;
	FJavaClassMethod InitializeCameraMethod;
	FJavaClassMethod TakePhotoMethod;
	FJavaClassMethod GetLastCapturedImageMethod;
	FJavaClassMethod SaveResultMethod;
	FJavaClassMethod ReleaseMethod;
	
};