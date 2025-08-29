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
	bool InitializeCamera(const FString& CameraId, uint8 AEMode, uint8 AFMode, uint8 AWBMode, uint8 ControMode, uint8 RotMode, int previewWidth, int previewHeight, int stillCaptureWidth, int stillCaptureHeight, int targetFPS);
    bool TakePhoto() ;
    bool GetLastCapturedImage(TArray<uint8>& OutJpeg) const;
	bool GetLastPreviewFrameInfo(void*& yPlaneBuffer, void*& uPlaneBuffer, void*& vPlaneBuffer, int32 & previewWidth, int32 & previewHeight) ;
    bool SaveResult(FString& OutAbsolutePath);
	void ReleaseLastPreviewFrameInfo();

private:
	static FName GetClassName();

	FJavaClassMethod GetCameraIdListMethod;
	FJavaClassMethod InitializeCameraMethod;
	FJavaClassMethod TakePhotoMethod;
	FJavaClassMethod getLastFrameInfoMethod;
	FJavaClassMethod releaseFrameInfoMethod;
	FJavaClassMethod GetLastCapturedImageMethod;
	FJavaClassMethod SaveResultMethod;
	FJavaClassMethod ReleaseMethod;
	
};