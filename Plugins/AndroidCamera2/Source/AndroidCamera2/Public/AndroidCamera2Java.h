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
	bool GetInitilizedCamaraState();
	void Release();
	
	// TODO: missing functionality for stillCapure
    bool TakePhoto() ;
    bool GetLastCapturedImage(TArray<uint8>& OutJpeg) const;
	bool SaveResult(FString& OutAbsolutePath);
	// END TODO

	bool GetLastPreviewFrameInfo(void*& yPlaneBuffer, void*& uPlaneBuffer, void*& vPlaneBuffer, int32 & previewWidth, int32 & previewHeight, int64& timeStamp) ;    
	void ReleaseLastPreviewFrameInfo();	
	int64 GetLastFrameTimeStamp();
	bool GetCameraIntrinsincs(const FString& CameraId, float& FocalLengthX, float& FocalLengthY, float& PrincipalPointX, float& PrincipalPointY, float& Skew, int32& activeSensorLeft, int32& activeSensorTop, int32& activeSensorRight,  int32& activeSensorBottom, float& focalLengthMm, float& SensorWidthMM, float& SensorHeightMM, int32& sensorOrientation);
	bool GetCameraLensPose(const FString& CameraId, float& quat_x, float& quat_y, float& quat_z, float& quat_w, float& loc_x, float& loc_y, float& loc_z, int& reference);

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
	FJavaClassMethod getIntrinsicsMethod;
	FJavaClassMethod getLensPoseMethod;
};