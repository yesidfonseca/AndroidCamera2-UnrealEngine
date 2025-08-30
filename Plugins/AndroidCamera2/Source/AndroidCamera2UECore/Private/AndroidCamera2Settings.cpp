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

UTextureRenderTarget2D* UAndroidCamera2Settings::ResolveDefaultRT_Y() const
{ return RenderTargetDataYPlane.RenderTarget2D.IsNull() ? nullptr : RenderTargetDataYPlane.RenderTarget2D.LoadSynchronous(); }

UTextureRenderTarget2D* UAndroidCamera2Settings::ResolveDefaultRT_U() const
{ return RenderTargetDataYPlane.RenderTarget2D.IsNull() ? nullptr : RenderTargetDataYPlane.RenderTarget2D.LoadSynchronous(); }

UTextureRenderTarget2D* UAndroidCamera2Settings::ResolveDefaultRT_V() const
{ return RenderTargetDataYPlane.RenderTarget2D.IsNull() ? nullptr : RenderTargetDataYPlane.RenderTarget2D.LoadSynchronous(); }
