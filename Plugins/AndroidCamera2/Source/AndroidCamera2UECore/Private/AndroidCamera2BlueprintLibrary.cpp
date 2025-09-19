// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 Yesid Fonseca


#include "AndroidCamera2BlueprintLibrary.h"
#include "Kismet/GameplayStatics.h" 
#include "AndroidCamera2Subsystem.h"


bool UAndroidCamera2BlueprintLibrary::InitializeCamera(const FString& CameraId, EAndroidCamera2AEMode AEMode, EAndroidCamera2AFMode AFMode, EAndroidCamera2AWBMode AWBMode, EAndroidCamera2ControlMode ControlMode,
    EAndroidCamera2RotationMode RotMode, int32 previewWidth, int32 previewHeight, int32 targetFPS)
{
    
    if (UGameInstance* GI = UGameplayStatics::GetGameInstance(GWorld))
    {
        if (auto* Cam2 = GI->GetSubsystem<UAndroidCamera2Subsystem>())
        {
            
			return Cam2->InitializeCamera(CameraId, AEMode, AFMode, AWBMode, ControlMode, RotMode, previewWidth, previewHeight, targetFPS);
        }
    }
    return false;
}

void UAndroidCamera2BlueprintLibrary::ResumeCapturing()
{
    if (UGameInstance* GI = UGameplayStatics::GetGameInstance(GWorld))
    {
        if (auto* Cam2 = GI->GetSubsystem<UAndroidCamera2Subsystem>())
        {
            return Cam2->ResumeCamera();
        }
    }
}

void UAndroidCamera2BlueprintLibrary::PauseCapturing()
{
    if (UGameInstance* GI = UGameplayStatics::GetGameInstance(GWorld))
    {
        if (auto* Cam2 = GI->GetSubsystem<UAndroidCamera2Subsystem>())
        {
            return Cam2->PauseCamera();
        }
    }
}

void UAndroidCamera2BlueprintLibrary::StopCapturing()
{
    if (UGameInstance* GI = UGameplayStatics::GetGameInstance(GWorld))
    {
        if (auto* Cam2 = GI->GetSubsystem<UAndroidCamera2Subsystem>())
        {
            return Cam2->StopCamera();
        }
    }
}


TArray<FString> UAndroidCamera2BlueprintLibrary::GetCameraIdList()
{
    TArray<FString>CameraList = TArray<FString>();

    if (UGameInstance* GI = UGameplayStatics::GetGameInstance(GWorld))
    {
        if (auto* Cam2 = GI->GetSubsystem<UAndroidCamera2Subsystem>())
        {
            CameraList = Cam2->GetCameraIdList();
        }
    }
    return CameraList;
}

EAndroidCamera2State UAndroidCamera2BlueprintLibrary::GetCameraState()
{
    if (UGameInstance* GI = UGameplayStatics::GetGameInstance(GWorld))
    {
        if (auto* Cam2 = GI->GetSubsystem<UAndroidCamera2Subsystem>())
        {
            return Cam2->GetCameraState();
        }
    }

    return EAndroidCamera2State::OFF;
}

bool UAndroidCamera2BlueprintLibrary::GetCameraIntrinsics(FString CameraId, FAndroidCamera2Intrinsics& Intrinsics)
{
	Intrinsics = FAndroidCamera2Intrinsics();
    if (UGameInstance* GI = UGameplayStatics::GetGameInstance(GWorld))
    {
        if (auto* Cam2 = GI->GetSubsystem<UAndroidCamera2Subsystem>())
        {
            return Cam2->GetCameraIntrinsics(CameraId, Intrinsics);
        }
    }
    return false;
}

FString UAndroidCamera2BlueprintLibrary::AndroidCamera2Intrinsics_ToString(const FAndroidCamera2Intrinsics& In)
{
    return In.ToString();
}
