#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h" // FTickableGameObject

#if PLATFORM_ANDROID
#include "AndroidCamera2Java.h"
#endif
#include "AndroidCamera2Subsystem.generated.h"

class UTextureRenderTarget2D;

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
                                , public FTickableGameObject
{
    GENERATED_BODY()
public:
    // Subsystem
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

	

    // FTickableGameObject (Tick en Game Thread)
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override
    { RETURN_QUICK_DECLARE_CYCLE_STAT(UCamera2UESubsystem, STATGROUP_Tickables); }
    virtual ETickableTickType GetTickableTickType() const override
    { return ETickableTickType::Always; }     // o Always si prefieres
    virtual bool IsTickable() const override
    { return true; }                            // controla si tiquea
    virtual bool IsTickableInEditor() const override
    { return false; }                               // true si quieres PIE/editor

	bool InitializeCamera(const FString& CameraId, EAndroidCamera2AEMode AEMode, EAndroidCamera2AFMode AFMode, EAndroidCamera2AWBMode AWBMode, EAndroidCamera2ControlMode ControlMode,
		EAndroidCamera2RotationMode RotMode, int32 previewWidth = 1280, int32 previewHeight = 720, int32 stillCaptureWidth = 1920, int32 stillCaptureHeight = 1080, int32 targetFPS = 30);

	// API de alto nivel (Game Thread)
    TArray<FString> GetCameraIdList();

	void GetLastFrameInfo();

private:
    bool bAutoUpdateRenderTargets = false;
	int64 LastFrameTimestamp = 0;
	//Camera State: 
	// 0=Not initialized, 
	// 1=Initialized, 
	// 2=Error during initialization
	// 3=Waiting for Initialization
	int32 CameraState = 0;

#if PLATFORM_ANDROID
    TSharedPtr<FAndroidCamera2Java, ESPMode::ThreadSafe> AndroidCamera2Java;
#endif

    bool IsValidAC2J();

	void UpdateRenderTextures();

	UPROPERTY() UTextureRenderTarget2D* y_RT2D = nullptr;
	UPROPERTY() UTextureRenderTarget2D* u_RT2D = nullptr;
	UPROPERTY() UTextureRenderTarget2D* v_RT2D = nullptr;
	bool bRenderYRT = false;
	bool bRenderURT = false;
	bool bRenderVRT = false;
	bool bUpdateYBuffer = false;
	bool bUpdateUBuffer = false;
	bool bUpdateVBuffer = false;
	TArray<uint8> YBuffer;
	TArray<uint8> UBuffer;
	TArray<uint8> VBuffer;
	int32 CurrentWidth = 0;
	int32 CurrentHeight = 0;

	UTextureRenderTarget2D* ValidateRenderTarget(TSoftObjectPtr<UTextureRenderTarget2D> RenderTarget2D);
};