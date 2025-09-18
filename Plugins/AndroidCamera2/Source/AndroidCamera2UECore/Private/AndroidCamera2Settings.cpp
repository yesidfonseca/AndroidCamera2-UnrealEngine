// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 Yesid Fonseca


#include "AndroidCamera2Settings.h"
#include "UObject/SoftObjectPath.h"

UAndroidCamera2Settings::UAndroidCamera2Settings()
{
    // Valores por defecto apuntando a RTs *del plugin* (ajusta rutas a tus assets)
    // OJO: usa rutas válidas de tus assets .uasset dentro del plugin.
    RenderTargetDataYPlane.RenderTarget2D = TSoftObjectPtr<UTextureRenderTarget2D>(
        FSoftObjectPath(TEXT("/AndroidCamera2/DefaultRenderTextures/YPlaneRT2D.YPlaneRT2D")));
    RenderTargetDataUPlane.RenderTarget2D = TSoftObjectPtr<UTextureRenderTarget2D>(
        FSoftObjectPath(TEXT("/AndroidCamera2/DefaultRenderTextures/UPlaneRT2D.UPlaneRT2D")));
    RenderTargetDataVPlane.RenderTarget2D = TSoftObjectPtr<UTextureRenderTarget2D>(
        FSoftObjectPath(TEXT("/AndroidCamera2/DefaultRenderTextures/VPlaneRT2D.VPlaneRT2D")));
}

#if WITH_EDITOR
void UAndroidCamera2Settings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
