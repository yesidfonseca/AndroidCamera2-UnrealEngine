#include "AndroidCamera2Java.h"
#include "Android/AndroidApplication.h"
#if WITH_LIBYUV
#include "libyuv.h" // o <libyuv/convert.h>, etc. según necesites
#endif

#if UE_BUILD_SHIPPING
// always clear any exceptions in SHipping
#define CHECK_JNI_RESULT(Id) if (Id == 0) { JEnv->ExceptionClear(); }
#else
#define CHECK_JNI_RESULT(Id) \
if (Id == 0) \
{ \
	if (bIsOptional) { JEnv->ExceptionClear(); } \
	else { JEnv->ExceptionDescribe(); checkf(Id != 0, TEXT("Failed to find " #Id)); } \
}
#endif

static jfieldID FindField(JNIEnv* JEnv, jclass Class, const ANSICHAR* FieldName, const ANSICHAR* FieldType, bool bIsOptional)
{
	jfieldID Field = Class == NULL ? NULL : JEnv->GetFieldID(Class, FieldName, FieldType);
	CHECK_JNI_RESULT(Field);
	return Field;
}

FAndroidCamera2Java::FAndroidCamera2Java():FJavaClassObject(GetClassName(), "()V")
{
	GetCameraIdListMethod = GetClassMethod("getCameraIdList", "()[Ljava/lang/String;");
	InitializeCameraMethod = GetClassMethod("initializeCamera", "(Ljava/lang/String;IIIIIIIIII)Z");
	TakePhotoMethod = GetClassMethod("takePhoto", "()Z"); 
	getLastFrameInfoMethod = GetClassMethod("getLastFrameInfo", "()Lcom/FonseCode/camera2/Camera2UE$FrameUpdateInfo;");
	GetLastCapturedImageMethod = GetClassMethod("getLastCapturedImage", "()[B"); 
	SaveResultMethod = GetClassMethod("saveResult", "()Ljava/lang/String;");
	ReleaseMethod = GetClassMethod("release", "()V");
	releaseFrameInfoMethod = GetClassMethod("releaseFrameInfo", "()V");
}

FAndroidCamera2Java::~FAndroidCamera2Java()
{
	CallMethod<void>(ReleaseMethod);
}

TArray<FString> FAndroidCamera2Java::GetCameraIdList() 
{
    TArray<FString> OutIds;

	JNIEnv* JEnv = FAndroidApplication::GetJavaEnv();
    jobjectArray CameraIdListArray = CallMethod<jobjectArray>(GetCameraIdListMethod);

	if (JEnv && JEnv->ExceptionCheck())
	{
		JEnv->ExceptionDescribe();   // imprime el stacktrace Java
		JEnv->ExceptionClear();      // limpia el estado de excepción de la VM
	}

	if (nullptr != CameraIdListArray)
	{
		
		jsize ElementCount = JEnv->GetArrayLength(CameraIdListArray);

		for (jsize i = 0; i < ElementCount; ++i)
		{
			jstring JStr = (jstring)JEnv->GetObjectArrayElement(CameraIdListArray, i);
			if (JStr)
			{
				const char* Utf = JEnv->GetStringUTFChars(JStr, nullptr);
				if (Utf) { OutIds.Add(UTF8_TO_TCHAR(Utf)); JEnv->ReleaseStringUTFChars(JStr, Utf); }
				JEnv->DeleteLocalRef(JStr);
			}
		}
		JEnv->DeleteGlobalRef(CameraIdListArray);
	}
	return OutIds;
}

bool FAndroidCamera2Java::InitializeCamera(const FString& CameraId, uint8 AEMode, uint8 AFMode, uint8 AWBMode, uint8 ControlMode, uint8 RotMode, int previewWidth, int previewHeight, int stillCaptureWidth, int stillCaptureHeight, int targetFPS)
{
	bool bOK = CallMethod<bool>(
		InitializeCameraMethod,
		*GetJString(CameraId),
		static_cast<jint>(AEMode),
		static_cast<jint>(AFMode),
		static_cast<jint>(AWBMode),
		static_cast<jint>(ControlMode),
		static_cast<jint>(RotMode),
		static_cast<jint>(previewWidth),
		static_cast<jint>(previewHeight),
		static_cast<jint>(stillCaptureWidth),
		static_cast<jint>(stillCaptureHeight),
		static_cast<jint>(targetFPS)
	);

	return bOK;
}

bool FAndroidCamera2Java::TakePhoto()
{
	return CallMethod<bool>(TakePhotoMethod);
}

bool FAndroidCamera2Java::GetLastCapturedImage(TArray<uint8>& OutJpeg) const
{
	return false;
}

bool FAndroidCamera2Java::GetLastPreviewFrameInfo(void*& yPlaneBuffer, int32& previewWidth, int32& previewHeight)
{
	// This can return an exception in some cases
	JNIEnv* JEnv = FAndroidApplication::GetJavaEnv();
	jobject Result = CallMethod<jobject>(getLastFrameInfoMethod);

	if (!Result)
	{
		return false;
	}
	jclass FrameUpdateInfoClass = FAndroidApplication::FindJavaClassGlobalRef("com/FonseCode/camera2/Camera2UE$FrameUpdateInfo");
	jfieldID FrameUpdateInfo_y = FindField(JEnv, FrameUpdateInfoClass, "y", "Ljava/nio/ByteBuffer;", false);
	auto buffer = JEnv->GetObjectField(Result, FrameUpdateInfo_y);
	if (buffer)
	{
		yPlaneBuffer = JEnv->GetDirectBufferAddress(buffer);
		jfieldID FrameUpdateInfo_imgWidth = FindField(JEnv, FrameUpdateInfoClass, "imgWidth", "I", false);
		jfieldID FrameUpdateInfo_imgHeight = FindField(JEnv, FrameUpdateInfoClass, "imgHeight", "I", false);
		previewWidth = (int32)JEnv->GetIntField(Result, FrameUpdateInfo_imgWidth);
		previewHeight = (int32)JEnv->GetIntField(Result, FrameUpdateInfo_imgHeight);
		return true;
	}

	return false;
}

bool FAndroidCamera2Java::SaveResult(FString& OutAbsolutePath)
{
	OutAbsolutePath = CallMethod<FString>(SaveResultMethod);
	return true;
}

void FAndroidCamera2Java::ReleaseLastPreviewFrameInfo()
{
	CallMethod<void>(releaseFrameInfoMethod);
}

FName FAndroidCamera2Java::GetClassName()
{
	return FName("com/FonseCode/camera2/Camera2UE");
}