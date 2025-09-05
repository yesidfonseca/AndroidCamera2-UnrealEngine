// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 Yesid Fonseca
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
	bool GetLastPreviewFrameInfo(void*& yPlaneBuffer, void*& uPlaneBuffer, void*& vPlaneBuffer, int32 & previewWidth, int32 & previewHeight, int64& timeStamp) ;
    bool SaveResult(FString& OutAbsolutePath);
	void ReleaseLastPreviewFrameInfo();
	void Release();
	bool GetInitilizedCamaraState();
	int64 GetLastFrameTimeStamp();

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
	FJavaClassMethod getInitializeCameraStateMethod;
	FJavaClassMethod getLastFrameTimeStampMethod;
	
};