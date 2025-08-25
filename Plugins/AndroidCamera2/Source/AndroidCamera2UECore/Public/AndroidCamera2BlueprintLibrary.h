#pragma once



#include "Kismet/BlueprintFunctionLibrary.h"
#include "AndroidCamera2BlueprintLibrary.generated.h"

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
	AUTO = 1 ,
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

	/** Libera recursos de Camera2 (cierra sesión/reader/hilos). Idóneo al salir o cambiar de nivel. */
	UFUNCTION(BlueprintCallable, Category="Android|Camera2", DisplayName="Release Camera2")
	static void Release();

	/** Libera recursos de Camera2 (cierra sesión/reader/hilos). Idóneo al salir o cambiar de nivel. */
	UFUNCTION(BlueprintCallable, Category = "Android|Camera2", DisplayName = "GetLastFrameInfo Camera2")
	static void GetLastFrameInfo(UTextureRenderTarget2D* RT);

private:
	UFUNCTION(BlueprintCallable, Category = "Android|Camera2", DisplayName = "AndroidCamera2 IsValid")
	static bool IsValidAC2J();
	static void UpdateYPlaneIntoRT(UTextureRenderTarget2D* RT, const void* Buffer, int32 SrcWidth, int32 SrcHeight);
};