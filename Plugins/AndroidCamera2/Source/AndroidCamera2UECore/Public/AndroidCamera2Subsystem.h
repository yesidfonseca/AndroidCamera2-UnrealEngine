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

USTRUCT(BlueprintType)
struct FAndroidCamera2Intrinsics
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "AndroidCamera2")
		FVector2f FocalLength = FVector2f::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category = "AndroidCamera2")
	FVector2f PrincipalPoint = FVector2f::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category = "AndroidCamera2")
		float Skew = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "AndroidCamera2")
		FIntPoint SensorSizePx = FIntPoint::ZeroValue;
	UPROPERTY(BlueprintReadOnly, Category = "AndroidCamera2")
		float FocalLengthMm = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "AndroidCamera2")
	FVector2f SensorSizeMM = FVector2f::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category = "AndroidCamera2")
		int32 SensorOrientation = 0;
	FAndroidCamera2Intrinsics() {}

	FString ToString() const
	{
		return FString::Printf(TEXT("FocalLength: (%.3f, %.3f), PrincipalPoint: (%.3f, %.3f), Skew: %.2f, SensorSizePx: (%d, %d), FocalLengthMm: %.3f, SensorSizeMM: (%.3f, %.3f), SensorOrientation: %d"),
			FocalLength.X, FocalLength.Y, PrincipalPoint.X, PrincipalPoint.Y, Skew, SensorSizePx.X, SensorSizePx.Y, FocalLengthMm, SensorSizeMM.X, SensorSizeMM.Y, SensorOrientation);
	}
};

UENUM()
enum class EAndroidCamera2LensPoseReference:uint8
{
	PRIMARY_CAMERA = 0,
	GYROSCOPE = 1,
	UNDEFINED = 2,
	AUTOMOTIVE = 3

};

USTRUCT(BlueprintType)
struct FAndroidCamera2LensPose
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "AndroidCamera2")
		FQuat OrientationDeviceCoord = FQuat::Identity;
	UPROPERTY(BlueprintReadOnly, Category = "AndroidCamera2")
	FVector LocationDeviceCoord = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "AndroidCamera2")
	FQuat OrientationUECoord = FQuat::Identity;
	UPROPERTY(BlueprintReadOnly, Category = "AndroidCamera2")
	FVector LocationUECoord = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "AndroidCamera2")
	EAndroidCamera2LensPoseReference LensPoseReference = EAndroidCamera2LensPoseReference::UNDEFINED;
	FAndroidCamera2LensPose() {}
	FString ToString() const
	{
		return FString::Printf(TEXT("OrientationDeviceCoor: (x=%.3f, y=%.3f, z=%.3f, w=%.3f), Location: (x=%.3f, y=%.3f, z=%.3f), Reference: %s"),
			OrientationDeviceCoord.X, OrientationDeviceCoord.Y, OrientationDeviceCoord.Z, OrientationDeviceCoord.W,
			LocationDeviceCoord.X, LocationDeviceCoord.Y, LocationDeviceCoord.Z,
			*UEnum::GetValueAsString(LensPoseReference));
	}
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

	bool GetCameraIntrinsics(FString CameraId, FAndroidCamera2Intrinsics& Intrinsics);

	bool GetCameraLensPose(FString CameraId, FAndroidCamera2LensPose& LensPose);

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