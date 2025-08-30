#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/TextureRenderTarget2D.h"
#include "AndroidCamera2Settings.generated.h"

USTRUCT()
struct FAndroidCamera2OutputDataSettings
{
    GENERATED_BODY()

    UPROPERTY(config, EditAnywhere, Category = "Output|RenderTarget", meta = (ToolTip = "If true, write RenderTarget2D"))
    bool bRender = true;

    UPROPERTY(config, EditAnywhere, Category = "Output|BufferTarget", meta = (ToolTip = "If false, then Render must to be false"))
    bool bCaptureBuffer = true;

    // Soft refs: el usuario puede asignar sus assets; si están vacíos, usas defaults del plugin.
    UPROPERTY(config, EditAnywhere, Category = "Output|RenderTarget", meta = (AllowedClasses = "/Script/Engine.TextureRenderTarget2D"))
    TSoftObjectPtr<UTextureRenderTarget2D> RenderTarget2D;
};


UCLASS(config=Game, defaultconfig, meta=(DisplayName="Android Camera2"))
class ANDROIDCAMERA2UECORE_API UAndroidCamera2Settings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    UAndroidCamera2Settings();

    // Categoría que verás en Project Settings
#if WITH_EDITOR
    virtual FText GetSectionText() const override
    { return NSLOCTEXT("AndroidCamera2", "SettingsName", "Android Camera2"); }
    virtual FText GetSectionDescription() const override
    { return NSLOCTEXT("AndroidCamera2", "SettingsDesc", "Default output and color settings for the Android Camera2 plugin."); }



    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    UPROPERTY(config, EditAnywhere, Category="Output|AndroidCamera2OutputDataSettings", meta=(DisplayName="Y Plane (Luma)"))
    FAndroidCamera2OutputDataSettings RenderTargetDataYPlane = FAndroidCamera2OutputDataSettings();

    UPROPERTY(config, EditAnywhere, Category="Output|AndroidCamera2OutputDataSettings", meta=(DisplayName = "Cb Plane (Chroma blue-difference)"))
    FAndroidCamera2OutputDataSettings RenderTargetDataUPlane = FAndroidCamera2OutputDataSettings();
	
	UPROPERTY(config, EditAnywhere, Category="Output|AndroidCamera2OutputDataSettings", meta = (DisplayName = "Cr Plane (Chroma red-difference)"))
    FAndroidCamera2OutputDataSettings RenderTargetDataVPlane = FAndroidCamera2OutputDataSettings();

	UPROPERTY(config, EditAnywhere, Category = "Output|AndroidCamera2OutputDataSettings", meta = (DisplayName = "Auto Update Render Targets"))
	bool bAutoUpdateRenderTargets = true;


    // Helpers
    UTextureRenderTarget2D* ResolveDefaultRT_Y() const;
    UTextureRenderTarget2D* ResolveDefaultRT_U() const;
    UTextureRenderTarget2D* ResolveDefaultRT_V() const;
};
