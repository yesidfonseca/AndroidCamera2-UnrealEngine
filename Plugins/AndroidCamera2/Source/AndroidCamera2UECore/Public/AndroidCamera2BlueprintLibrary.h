#pragma once



#include "Kismet/BlueprintFunctionLibrary.h"
#include "AndroidCamera2BlueprintLibrary.generated.h"

class UTextureRenderTarget2D;
// Auto White Balance (AWB) modes — mirror de CameraMetadata.CONTROL_AWB_MODE_*



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

	/**
	 * Dispara una captura (TEMPLATE_STILL_CAPTURE). El resultado queda en memoria interna de la capa Java.
	 * Usa GetLastCapturedImage o SaveResult para recuperarlo.
	 * @return true si la solicitud de captura se envió correctamente.
	 */
	UFUNCTION(BlueprintCallable, Category="Android|Camera2", DisplayName="Take Photo")
	static bool TakePhoto();

	/**
	 * Copia los bytes JPEG de la última captura desde Java.
	 * @param OutJpegBytes  Arreglo de bytes JPEG (puede estar vacío si no hay captura previa).
	 * @return true si se obtuvieron bytes desde Java (aunque sea longitud 0); false si hubo error de plataforma/capa.
	 */
	UFUNCTION(BlueprintCallable, Category="Android|Camera2", DisplayName="Get Last Captured Image (Bytes)")
	static bool GetLastCapturedImage(TArray<uint8>& OutJpegBytes);

	/**
	 * Guarda la última captura en el almacenamiento interno (filesDir) con timestamp.
	 * @param OutAbsolutePath  Ruta absoluta del archivo guardado (".jpg") si tuvo éxito.
	 * @return true si se escribió el archivo correctamente.
	 */
	UFUNCTION(BlueprintCallable, Category="Android|Camera2", DisplayName="Save Last Capture To File")
	static bool SaveResult(FString& OutAbsolutePath);



private:
};