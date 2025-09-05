// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 Yesid Fonseca
#pragma once

#if PLATFORM_ANDROID
#include "CoreMinimal.h"

class ANDROIDCAMERA2_API FAndroidCamera2Manager
{
public:
	static void CapturePhoto();
};

#endif