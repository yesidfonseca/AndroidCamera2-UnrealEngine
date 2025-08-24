#include "AndroidCamera2Java.h"
#include "Android/AndroidApplication.h"
#if WITH_LIBYUV
#include "libyuv.h" // o <libyuv/convert.h>, etc. según necesites
#endif

FAndroidCamera2Java::FAndroidCamera2Java():FJavaClassObject(GetClassName(), "()V")
{
	GetCameraIdListMethod = GetClassMethod("getCameraIdList", "()[Ljava/lang/String;");
	InitializeCameraMethod = GetClassMethod("initializeCamera", "(Ljava/lang/String;IIII)Z");
	TakePhotoMethod = GetClassMethod("takePhoto", "()Z"); 
	GetLastCapturedImageMethod = GetClassMethod("getLastCapturedImage", "()[B"); 
	SaveResultMethod = GetClassMethod("saveResult", "()Ljava/lang/String;");
	ReleaseMethod = GetClassMethod("release", "()V");
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

bool FAndroidCamera2Java::InitializeCamera(const FString& CameraId, uint8 AEMode, uint8 AFMode, uint8 AWBMode, uint8 ControlMode)
{
	bool bOK = CallMethod<bool>(
		InitializeCameraMethod,
		*GetJString(CameraId),
		static_cast<jint>(AEMode),
		static_cast<jint>(AFMode),
		static_cast<jint>(AWBMode),
		static_cast<jint>(ControlMode)
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

bool FAndroidCamera2Java::SaveResult(FString& OutAbsolutePath)
{
	OutAbsolutePath = CallMethod<FString>(SaveResultMethod);
	return true;
}

FName FAndroidCamera2Java::GetClassName()
{
	return FName("com/FonseCode/camera2/Camera2UE");
}