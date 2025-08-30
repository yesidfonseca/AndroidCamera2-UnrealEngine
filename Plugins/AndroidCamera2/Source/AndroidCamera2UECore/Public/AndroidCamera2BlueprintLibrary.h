#pragma once



#include "Kismet/BlueprintFunctionLibrary.h"
#include "AndroidCamera2BlueprintLibrary.generated.h"

class UTextureRenderTarget2D;




/**
 * Funciones de alto nivel para usar Camera2 desde UE5.
 * - En Android: llaman a la capa JNI (wrapper FJavaAndroidCamera2 en el módulo Android).
 * - En otras plataformas: devuelven valores por defecto y registran un log amistoso.
 */
UCLASS()
class ANDROIDCAMERA2UECORE_API UAndroidCamera2BlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Devuelve la lista de CameraId disponibles reportadas por CameraManager (p.ej. "0", "1"). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Android|Camera2", DisplayName="Get Camera Id List")
	static TArray<FString> GetCameraIdList();

	/**
	 * Inicializa Camera2 con un CameraId específico (de la lista anterior).
	 * @param CameraId  Id válido (exacto) de getCameraIdList.
	 * @return true si se inició la apertura/configuración correctamente.
	 */
	UFUNCTION(BlueprintCallable, Category="Android|Camera2", DisplayName="Initialize Camera (by Id)")
	static bool InitializeCamera(const FString& CameraId, EAndroidCamera2AEMode AEMode, EAndroidCamera2AFMode AFMode, EAndroidCamera2AWBMode AWBMode, EAndroidCamera2ControlMode ControlMode,
		EAndroidCamera2RotationMode RotMode, int32 previewWidth = 1280, int32 previewHeight = 720, int32 stillCaptureWidth = 1920, int32 stillCaptureHeight =1080, int32 targetFPS =30);

	

};