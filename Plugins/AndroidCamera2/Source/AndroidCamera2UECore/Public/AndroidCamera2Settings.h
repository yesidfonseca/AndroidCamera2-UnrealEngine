// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 Yesid Fonseca
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/TextureRenderTarget2D.h"
#include "AndroidCamera2Settings.generated.h"

USTRUCT()
struct FAndroidCamera2OutputDataSettings
{
    GENERATED_BODY()

    UPROPERTY(config, EditAnywhere, Category = "Output|RenderTarget", meta = (ToolTip = "If true, you need to set RenderTarget2D"))
    bool bRender = true;

    UPROPERTY(config, EditAnywhere, Category = "Output|BufferTarget", meta = (ToolTip = "If false, then Render must to be false"))
    bool bCaptureBuffer = true;

    // Soft refs: el usuario puede asignar sus assets; si están vacíos, usas defaults del plugin.
    UPROPERTY(config, EditAnywhere, Category = "Output|RenderTarget", meta = (AllowedClasses = "/Script/Engine.TextureRenderTarget2D"))
    TSoftObjectPtr<UTextureRenderTarget2D> RenderTarget2D;
};


UCLASS(config = AndroidCamera2, defaultconfig, meta = (DisplayName = "Android Camera2"))
class ANDROIDCAMERA2UECORE_API UAndroidCamera2Settings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    UAndroidCamera2Settings();

    virtual FName GetContainerName() const override { return TEXT("Project"); } 
    virtual FName GetCategoryName()  const override { return TEXT("Plugins"); }
    virtual FName GetSectionName()   const override { return TEXT("Android Camera2"); }


    // Categoría que verás en Project Settings
#if WITH_EDITOR
    virtual FText GetSectionText() const override
    { return NSLOCTEXT("AndroidCamera2", "SectionTitle", "Android Camera2"); }
    virtual FText GetSectionDescription() const override
    { return NSLOCTEXT("AndroidCamera2", "SettingsDesc", "Camera2 capture and output settings."); }



    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    UPROPERTY(config, EditAnywhere, Category="Render and Buffering Settings", meta=(DisplayName="Y Plane (Luma)"))
    FAndroidCamera2OutputDataSettings RenderTargetDataYPlane = FAndroidCamera2OutputDataSettings();

    UPROPERTY(config, EditAnywhere, Category="Render and Buffering Settings", meta=(DisplayName = "Cb Plane (Chroma blue-difference)"))
    FAndroidCamera2OutputDataSettings RenderTargetDataUPlane = FAndroidCamera2OutputDataSettings();
	
	UPROPERTY(config, EditAnywhere, Category="Render and Buffering Settings", meta = (DisplayName = "Cr Plane (Chroma red-difference)"))
    FAndroidCamera2OutputDataSettings RenderTargetDataVPlane = FAndroidCamera2OutputDataSettings();

    UPROPERTY(config, EditAnywhere, Category = "Camera Settings", meta = (DisplayName = "Time Out in seconds of camera after initizialization"))
    float CameraTimeOut = 5.f;

    UPROPERTY(config, EditAnywhere, Category = "Permissions Meta Quest", meta = (DisplayName = "Request Headset Camera Permission"))
    bool bRequestHeadsetCameraPermission = false;
};
