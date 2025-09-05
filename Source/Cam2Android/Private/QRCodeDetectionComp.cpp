// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 Yesid Fonseca


#include "QRCodeDetectionComp.h"
#include "AndroidCamera2Subsystem.h"
#include "Kismet/GameplayStatics.h" 
#include "Engine/Engine.h"

// Sets default values for this component's properties
UQRCodeDetectionComp::UQRCodeDetectionComp()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	// ...
}


// Called when the game starts
void UQRCodeDetectionComp::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UQRCodeDetectionComp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	bool bGotFrame = false;
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(GWorld))
	{
		if (auto* Cam2 = GI->GetSubsystem<UAndroidCamera2Subsystem>())
		{
			if (Cam2->GetCameraState() == EAndroidCamera2State::INITIALIZED)
			{
				const uint8* YPtr = nullptr;
				int32 YW = 0, YH = 0;
				int64 YTs = 0;
				if (Cam2->GetLuminanceBufferPtr(YPtr, YW, YH, YTs))
				{
					// comprobar dimensiones
					if (Width != YW || Height != YH || YCurr.Num() != Width * Height)
					{
						Width = YW; Height = YH;
						InitializeLuma(Width, Height);
					}

					if (YTs <= LastFrameTimestamp)
					{
						return;
					}

					// copiar luminancia
					FMemory::Memcpy(YCurr.GetData(), YPtr, Width * Height);
					LastFrameTimestamp = YTs;
					bGotFrame = true;

					FQuircReader::DecodeFromLuma(YCurr.GetData(), Width, Height, Width, QRDetections);

				}
			}
		}

	}

	if (bGotFrame == false)
	{
		// no hay frame nuevo, salir
		return;
	}

	if (OnQRCodeDetected.IsBound())
		OnQRCodeDetected.Broadcast(QRDetections);
}



void UQRCodeDetectionComp::InitializeLuma(int32 InWidth, int32 InHeight)
{
	check(InWidth > 0 && InHeight > 0);
	Width = InWidth;
	Height = InHeight;

	YCurr.SetNumZeroed(Width * Height);
}