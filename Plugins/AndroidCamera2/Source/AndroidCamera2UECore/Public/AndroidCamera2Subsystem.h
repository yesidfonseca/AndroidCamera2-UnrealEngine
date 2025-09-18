// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 Yesid Fonseca
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h" 
#include "Templates/SharedPointer.h"



#include "AndroidCamera2Subsystem.generated.h"

class UTextureRenderTarget2D;
class FAndroidCamera2ThreadSafe;
class FAndroidCamera2ClockSink;

UENUM(BlueprintType)
enum class EAndroidCamera2State : uint8
{
	OFF,
	WAITING_INIT,
	INITIALIZED,
	FAIL_INIT, 
	PAUSED
};

// Auto White Balance (AWB) modes — mirror de CameraMetadata.CONTROL_AWB_MODE_*
UENUM(BlueprintType)
enum class EAndroidCamera2AWBMode : uint8
{
	OFF = 0,
	AUTO = 1,
	INCANDESCENT = 2,
	FLUORESCENT = 3,
	WARM_FLUORESCENT = 4,
	DAYLIGHT = 5,
	CLOUDY_DAYLIGHT = 6,
	TWILIGHT = 7,
	SHADE = 8
};

// Auto Exposure (AE) modes — mirror de CameraMetadata.CONTROL_AE_MODE_*
UENUM(BlueprintType)
enum class EAndroidCamera2AEMode : uint8
{
	OFF = 0,
	ON = 1,
	ON_AUTO_FLASH = 2,
	ON_ALWAYS_FLASH = 3,
	ON_AUTO_FLASH_REDEYE = 4,
	ON_EXTERNAL_FLASH = 5
};

// Auto Focus (AF) modes — mirror de CameraMetadata.CONTROL_AF_MODE_*
UENUM(BlueprintType)
enum class EAndroidCamera2AFMode : uint8
{
	OFF = 0,
	AUTO = 1,
	MACRO = 2,
	CONTINUOUS_VIDEO = 3,
	CONTINUOUS_PICTURE = 4,
	EDOF = 5
};

// Control Mode — mirror de CameraMetadata.CONTROL_MODE_*
UENUM(BlueprintType)
enum class EAndroidCamera2ControlMode : uint8
{
	OFF = 0,
	AUTO = 1,
	USE_SCENE_MODE = 2,
	OFF_KEEP_STATE = 3,
	USE_EXTENDED_SCENE_MODE = 4
};

UENUM(BlueprintType)
enum class EAndroidCamera2RotationMode : uint8
{
	R0 = 0,
	R90 = 1,
	R180 = 2,
	R270 = 3,
	RSensor = 4
};


UCLASS()
class ANDROIDCAMERA2UECORE_API UAndroidCamera2Subsystem final : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    // Subsystem
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;


	UAndroidCamera2Subsystem();


    virtual void TickFetch(FTimespan DeltaTime);


	//TODO: missing functionality for stillCapure
	bool InitializeCamera(const FString& CameraId, EAndroidCamera2AEMode AEMode, EAndroidCamera2AFMode AFMode, EAndroidCamera2AWBMode AWBMode, EAndroidCamera2ControlMode ControlMode,
		EAndroidCamera2RotationMode RotMode, int32 previewWidth = 1280, int32 previewHeight = 720, int32 targetFPS = 30);

	
    TArray<FString> GetCameraIdList();

	

	bool GetLuminanceBufferPtr(const uint8*& OutPtr, int32& OutWidth, int32& OutHeight, int64& OutTimestamp) const;

	bool GetCbChromaBufferPtr(const uint8*& OutPtr, int32& OutWidth, int32& OutHeight, int64& OutTimestamp) const;

	bool GetCrChromaBufferPtr(const uint8*& OutPtr, int32& OutWidth, int32& OutHeight, int64& OutTimestamp) const;

	void SetCameraTimeout(float NewTimeout);

	EAndroidCamera2State GetCameraState() const { return CameraState; }

	void PauseCamera();

	void ResumeCamera();

	void StopCamera();

private:
	EAndroidCamera2State CameraState = EAndroidCamera2State::OFF;



	float CameraTimeout = 5.0f; // seconds
	float CameraTimeLeftAfterInitialization = 5.f;

	UPROPERTY() UTextureRenderTarget2D* y_RT2D = nullptr;
	UPROPERTY() UTextureRenderTarget2D* u_RT2D = nullptr;
	UPROPERTY() UTextureRenderTarget2D* v_RT2D = nullptr;
	
	//TArray<uint8> YBuffer;
	//TArray<uint8> UBuffer;
	//TArray<uint8> VBuffer;
	//int32 CurrentWidth = 0;
	//int32 CurrentHeight = 0;

	
	TSharedPtr<FAndroidCamera2ThreadSafe, ESPMode::ThreadSafe> AndroidCamera2;

	/** The recorder's media clock sink. */
	TSharedPtr<FAndroidCamera2ClockSink, ESPMode::ThreadSafe> ClockSink;

	UTextureRenderTarget2D* ValidateRenderTarget(TSoftObjectPtr<UTextureRenderTarget2D> RenderTarget2D);

	void UpdateRenderTextures();

	static void UpdatePlaneTexture_RenderThread(FRHICommandListImmediate& RHICmd, FTextureRenderTargetResource* RTRes, const uint8* Src, int32 W, int32 H);

};