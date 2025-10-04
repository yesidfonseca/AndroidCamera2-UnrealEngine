// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 Yesid Fonseca



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
	getInitializeCameraStateMethod = GetClassMethod("getInitializeCameraState", "()Z");
	getLastFrameTimeStampMethod = GetClassMethod("getLastFrameTimeStamp", "()J");
	getIntrinsicsMethod = GetClassMethod("getIntrinsics", "(Ljava/lang/String;)Lcom/FonseCode/camera2/Camera2UE$Intrinsics;");
	getLensPoseMethod = GetClassMethod("getLensPose", "(Ljava/lang/String;)Lcom/FonseCode/camera2/Camera2UE$LensPose;");
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

bool FAndroidCamera2Java::GetLastPreviewFrameInfo(void*& yPlaneBuffer, void*& uPlaneBuffer, void*& vPlaneBuffer, int32& previewWidth, int32& previewHeight, int64& timeStamp)
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
	jfieldID FrameUpdateInfo_u = FindField(JEnv, FrameUpdateInfoClass, "u", "Ljava/nio/ByteBuffer;", false);
	jfieldID FrameUpdateInfo_v = FindField(JEnv, FrameUpdateInfoClass, "v", "Ljava/nio/ByteBuffer;", false);
	auto ybuffer = JEnv->GetObjectField(Result, FrameUpdateInfo_y);
	auto ubuffer = JEnv->GetObjectField(Result, FrameUpdateInfo_u);
	auto vbuffer = JEnv->GetObjectField(Result, FrameUpdateInfo_v);
	if (ybuffer && ubuffer && vbuffer)
	{
		yPlaneBuffer = JEnv->GetDirectBufferAddress(ybuffer);
		uPlaneBuffer = JEnv->GetDirectBufferAddress(ubuffer);
		vPlaneBuffer = JEnv->GetDirectBufferAddress(vbuffer);
		jfieldID FrameUpdateInfo_imgWidth = FindField(JEnv, FrameUpdateInfoClass, "imgWidth", "I", false);
		jfieldID FrameUpdateInfo_imgHeight = FindField(JEnv, FrameUpdateInfoClass, "imgHeight", "I", false);
		jfieldID FrameUpdateInfo_timeStamp = FindField(JEnv, FrameUpdateInfoClass, "timeStamp", "J", false);
		previewWidth = (int32)JEnv->GetIntField(Result, FrameUpdateInfo_imgWidth);
		previewHeight = (int32)JEnv->GetIntField(Result, FrameUpdateInfo_imgHeight);
		timeStamp = (int64)JEnv->GetLongField(Result, FrameUpdateInfo_timeStamp);
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

void FAndroidCamera2Java::Release()
{
	CallMethod<void>(ReleaseMethod);
}

bool FAndroidCamera2Java::GetInitilizedCamaraState()
{
	return CallMethod<bool>(getInitializeCameraStateMethod);
}

int64 FAndroidCamera2Java::GetLastFrameTimeStamp()
{
	return CallMethod<int64>(getLastFrameTimeStampMethod);
}

bool FAndroidCamera2Java::GetCameraIntrinsincs(const FString& CameraId, float& FocalLengthX, float& FocalLengthY, float& PrincipalPointX, float& PrincipalPointY, float& Skew, int32& activeSensorLeft, int32& activeSensorTop, int32& activeSensorRight, int32& activeSensorBottom, float& focalLengthMm, float& SensorWidthMM, float& SensorHeightMM, int32& sensorOrientation)
{
	// This can return an exception in some cases
	JNIEnv* JEnv = FAndroidApplication::GetJavaEnv();
	jobject Result = CallMethod<jobject>(getIntrinsicsMethod, *GetJString(CameraId));

	if (!Result)
	{
		return false;
	}

	jclass IntrinsicsClass = FAndroidApplication::FindJavaClassGlobalRef("com/FonseCode/camera2/Camera2UE$Intrinsics");
	jfieldID Intrinsics_fx = FindField(JEnv, IntrinsicsClass,					"fx",				"F", false);
	jfieldID Intrinsics_fy = FindField(JEnv, IntrinsicsClass,					"fy",				"F", false);
	jfieldID Intrinsics_cx = FindField(JEnv, IntrinsicsClass,					"cx",				"F", false);
	jfieldID Intrinsics_cy = FindField(JEnv, IntrinsicsClass,					"cy",				"F", false);
	jfieldID Intrinsics_skew = FindField(JEnv, IntrinsicsClass,					"skew",				"F", false);
	jfieldID Intrinsics_activeSensorLeft = FindField(JEnv, IntrinsicsClass,		"activeSensorLeft",	"I", false);
	jfieldID Intrinsics_activeSensorRight = FindField(JEnv, IntrinsicsClass,	"activeSensorRight","I", false);
	jfieldID Intrinsics_activeSensorTop = FindField(JEnv, IntrinsicsClass,		"activeSensorTop",	"I", false);
	jfieldID Intrinsics_activeSensorBottom = FindField(JEnv, IntrinsicsClass,	"activeSensorBottom","I", false);
	jfieldID Intrinsics_focalLengthMm = FindField(JEnv, IntrinsicsClass,		"focalLengthMm",	"F", false);
	jfieldID Intrinsics_sensorWidthMm = FindField(JEnv, IntrinsicsClass,		"sensorWidthMm",	"F", false);
	jfieldID Intrinsics_sensorHeightMm = FindField(JEnv, IntrinsicsClass,		"sensorHeightMm",	"F", false);
	jfieldID Intrinsics_sensorOrientation = FindField(JEnv, IntrinsicsClass,	"sensorOrientation","I", false);

	FocalLengthX = JEnv->GetFloatField(Result,		Intrinsics_fx);
	FocalLengthY = JEnv->GetFloatField(Result,		Intrinsics_fy);
	PrincipalPointX = JEnv->GetFloatField(Result,	Intrinsics_cx);
	PrincipalPointY = JEnv->GetFloatField(Result,	Intrinsics_cy);
	Skew = JEnv->GetFloatField(Result,				Intrinsics_skew);
	activeSensorLeft = JEnv->GetIntField(Result,	Intrinsics_activeSensorLeft);
	activeSensorRight = JEnv->GetIntField(Result,	Intrinsics_activeSensorRight);
	activeSensorTop = JEnv->GetIntField(Result,		Intrinsics_activeSensorTop);
	activeSensorBottom = JEnv->GetIntField(Result,	Intrinsics_activeSensorBottom);
	focalLengthMm = JEnv->GetFloatField(Result,		Intrinsics_focalLengthMm);
	SensorWidthMM = JEnv->GetFloatField(Result,		Intrinsics_sensorWidthMm);
	SensorHeightMM = JEnv->GetFloatField(Result,	Intrinsics_sensorHeightMm);
	sensorOrientation = JEnv->GetIntField(Result,	Intrinsics_sensorOrientation);

	return true;
}

bool FAndroidCamera2Java::GetCameraLensPose(const FString& CameraId, float& quat_x, float& quat_y, float& quat_z, float& quat_w, float& loc_x, float& loc_y, float& loc_z, int& reference)
{
	// This can return an exception in some cases
	JNIEnv* JEnv = FAndroidApplication::GetJavaEnv();
	jobject Result = CallMethod<jobject>(getLensPoseMethod, *GetJString(CameraId));

	if (!Result)
	{
		return false;
	}

	jclass LensPoseClass = FAndroidApplication::FindJavaClassGlobalRef("com/FonseCode/camera2/Camera2UE$LensPose");
	jfieldID LensPose_quat_x	= FindField(JEnv, LensPoseClass, "quat_x", "F", false);
	jfieldID LensPose_quat_y	= FindField(JEnv, LensPoseClass, "quat_y", "F", false);
	jfieldID LensPose_quat_z	= FindField(JEnv, LensPoseClass, "quat_z", "F", false);
	jfieldID LensPose_quat_w	= FindField(JEnv, LensPoseClass, "quat_w", "F", false);
	jfieldID LensPose_loc_x		= FindField(JEnv, LensPoseClass, "loc_x", "F", false);
	jfieldID LensPose_loc_y		= FindField(JEnv, LensPoseClass, "loc_y", "F", false);
	jfieldID LensPose_loc_z		= FindField(JEnv, LensPoseClass, "loc_z", "F", false);
	jfieldID LensPose_reference = FindField(JEnv, LensPoseClass, "lensposeReference", "I", false);

	quat_x = JEnv->GetFloatField(Result, LensPose_quat_x);
	quat_y = JEnv->GetFloatField(Result, LensPose_quat_y);
	quat_z = JEnv->GetFloatField(Result, LensPose_quat_z);
	quat_w = JEnv->GetFloatField(Result, LensPose_quat_w);
	loc_x = JEnv->GetFloatField(Result, LensPose_loc_x);
	loc_y = JEnv->GetFloatField(Result, LensPose_loc_y);
	loc_z = JEnv->GetFloatField(Result, LensPose_loc_z);
	reference = JEnv->GetIntField(Result, LensPose_reference);

	return true;
}

