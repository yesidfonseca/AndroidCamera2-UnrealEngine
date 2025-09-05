// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 Yesid Fonseca
#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "AndroidCamera2BlueprintLibrary.generated.h"

class UTextureRenderTarget2D;


UCLASS()
class ANDROIDCAMERA2UECORE_API UAndroidCamera2BlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Android|Camera2", DisplayName="Get Camera Id List")
	static TArray<FString> GetCameraIdList();

	
	
	UFUNCTION(BlueprintCallable, Category="Android|Camera2", DisplayName="Initialize Camera (by Id)")
	static bool InitializeCamera(const FString& CameraId, EAndroidCamera2AEMode AEMode, EAndroidCamera2AFMode AFMode, EAndroidCamera2AWBMode AWBMode, EAndroidCamera2ControlMode ControlMode,
		EAndroidCamera2RotationMode RotMode, int32 previewWidth = 1280, int32 previewHeight = 720, int32 targetFPS =30);

	
	UFUNCTION(BlueprintCallable, Category = "Android|Camera2", DisplayName = "StopCapturing")
	static void StopCapturing();

	UFUNCTION(BlueprintCallable, Category = "Android|Camera2", DisplayName = "ResumeCapturing")
	static void ResumeCapturing();

	UFUNCTION(BlueprintCallable, Category = "Android|Camera2", DisplayName = "PauseCapturing")
	static void PauseCapturing();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Android|Camera2", DisplayName = "GetCameraState")
	static EAndroidCamera2State GetCameraState();
};