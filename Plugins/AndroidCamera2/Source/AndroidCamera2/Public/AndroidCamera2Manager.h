#pragma once

#if PLATFORM_ANDROID
#include "CoreMinimal.h"

class ANDROIDCAMERA2_API FAndroidCamera2Manager
{
public:
	static void CapturePhoto();
};

#endif