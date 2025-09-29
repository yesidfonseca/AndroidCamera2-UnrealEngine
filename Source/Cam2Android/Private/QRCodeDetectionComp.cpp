// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 Yesid Fonseca

#include "QRCodeDetectionComp.h"
#include "Kismet/GameplayStatics.h" 
#include "Camera/CameraComponent.h"
#include "AndroidCamera2Subsystem.h"
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
	LastFrameTimestamp = 0;

	LastCamPoses.SetNum(CameraSamples);
	LastCamPosesTimes.SetNum(CameraSamples);
	QRCodeCornersRay.SetNum(4);
	FTriangulationSolver Solver = FTriangulationSolver();
	Solver.LineMaxSamples = QRCodeTriangulationSolverSamples;
	QRCodeCornerTSolvers.Init(Solver, 4);
	EstimatedCornersLocations.SetNum(4);

}


// Called every frame
void UQRCodeDetectionComp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	static bool bCamInitialized = false;
	
	static FAndroidCamera2Intrinsics CamLensIntrinsics = FAndroidCamera2Intrinsics();

	static FQuat CamLensPoseRot = FQuat::Identity;

	bool bGotFrame = false;
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(GWorld))
	{
		if (auto* Cam2 = GI->GetSubsystem<UAndroidCamera2Subsystem>())
		{		
			if (Cam2->GetCameraState() == EAndroidCamera2State::INITIALIZED)
			{
				static int32 CameraSampleIdx = 0;
				if (Camera)
				{
					FTransform CamHDM = Camera->GetComponentToWorld();
					FRotator LeftEyeRot;
					FVector LeftEyeLoc;
					GetLeftEyeWorldPos(CamHDM, LeftEyeRot, LeftEyeLoc);
					LastCamPoses[CameraSampleIdx] = FTransform(LeftEyeRot, LeftEyeLoc);
					LastCamPosesTimes[CameraSampleIdx] = FPlatformTime::Cycles64();
					CameraSampleIdx = (CameraSampleIdx + 1) % CameraSamples;
				}

				if(!bCamInitialized)
				{
					FAndroidCamera2LensPose CamLensPose;
					Cam2->GetCameraIntrinsics(FString("50"), CamLensIntrinsics);
					Cam2->GetCameraLensPose(FString("50"), CamLensPose);
					CamLensPoseRot = FRotator(CamLensPose.OrientationUECoord.Rotator().Pitch, 0, 0).Quaternion();
					bCamInitialized = true;
				}

				for (int32 i = 0; i < 4; i++)
				{
					EstimatedCornersLocations[i] = QRCodeCornerTSolvers[i].ComputeEstimatedPoint();
				}
				//UE_LOG(LogTemp, Display, TEXT("UQRCodeDetectionComp::TickComponent. EstP:%s, Delta:%s, Step:%f"), *EstimatedPoses[0].ToString(), *QRCodeCornerTSolvers[0].Delta.ToString(), QRCodeCornerTSolvers[0].Step);
				

				const uint8* YPtr = nullptr;
				int32 YW = 0, YH = 0;
				uint64 YTs = 0;
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

	if(QRDetections.Num()>0)
	{
		UpdateCameraCachePose();

		auto CameraCachePosePrevious = LastCamPoses[(CameraPoseIndex +CameraSamples-1) % CameraSamples];
		const float PosDelta = FVector::Dist(CameraCachePose.GetLocation(), CameraCachePosePrevious.GetLocation());		
		const float AngleRad = FQuat::ErrorAutoNormalize(CameraCachePose.GetRotation().GetNormalized(), CameraCachePosePrevious.GetRotation().GetNormalized()); // distancia angular en rad
		const float AngleDeg = FMath::RadiansToDegrees(AngleRad);

		//Avoid analizing when camera is moving too much
		if (PosDelta > .05f || AngleDeg > .05f)
			return;
		//UE_LOG(LogTemp, Display, TEXT("UQRCodeDetectionComp::TickComponent. PosDelta:%f, AngleDeg: %f"), PosDelta, AngleDeg);

		FRotator Rot = (CameraCachePose.Rotator().Quaternion() * CamLensPoseRot).Rotator();
		for (int32 i = 0; i< QRDetections[0].Corners.Num(); i++)
		{
			auto Corner = QRDetections[0].Corners[i];
			auto C = (FVector2f(Corner) - CamLensIntrinsics.PrincipalPoint)/CamLensIntrinsics.FocalLength;
			FVector Ray = FVector(1, C.X, -C.Y);
			Ray.Normalize();
			Ray = Rot.RotateVector(Ray);
			QRCodeCornersRay[i] = Ray;
			
		}

		if (FVector::Dist(CameraCachePose.GetLocation(), PreviousCameraCachePoseLocation) < 5.f)
		{
			const float AngleDeg2 = (180.0) / UE_DOUBLE_PI * FMath::Acos(FVector::DotProduct(QRCodeCornersRay[0], PreviousCornerRay));
			//Avoid analizing when QRCodeRay is not changing too much the angle
			if(AngleDeg2<10.f)
				return;
		}

		PreviousCameraCachePoseLocation = CameraCachePose.GetLocation();
		PreviousCornerRay = QRCodeCornersRay[0];

		for (int32 i = 0; i <4; i++)
		{
			QRCodeCornerTSolvers[i].AddLine(CameraCachePose.GetLocation(), QRCodeCornersRay[i]);
		}

		OnQRUsedForPoseEstimation();
	}

	

	

	
}

void UQRCodeDetectionComp::OnQRUsedForPoseEstimation_Implementation()
{
}



void UQRCodeDetectionComp::InitializeLuma(int32 InWidth, int32 InHeight)
{
	check(InWidth > 0 && InHeight > 0);
	Width = InWidth;
	Height = InHeight;

	YCurr.SetNumZeroed(Width * Height);
}

void UQRCodeDetectionComp::UpdateCameraCachePose()
{
	// Search the camera pose with the nearest timestamp to LastFrameTimestamp
	double MinDistanceTimeInSeconds = 1000000000.f;
	double LastFrameTimestampInSeconds = FPlatformTime::ToSeconds64(LastFrameTimestamp);
	double CameraPoseTimeInSeconds = 0;
	int32 bestIdx = 0;
	for (int i = 0; i < CameraSamples; i++)
	{
		CameraPoseTimeInSeconds = FPlatformTime::ToSeconds64(LastCamPosesTimes[i]);
		double CurrentDistanceTimeInSeconds = FMath::Abs(CameraPoseTimeInSeconds - LastFrameTimestampInSeconds);
		if (CurrentDistanceTimeInSeconds < MinDistanceTimeInSeconds)
		{
			bestIdx = i;
			MinDistanceTimeInSeconds = CurrentDistanceTimeInSeconds;
		}
	}
	//UE_LOG(LogTemp, Display, TEXT("UQRCodeDetectionComp::UpdateCameraCachePose. LastFrameTimestamp:%.6f, BestTimestamp:%.5f"), LastFrameTimestampInSeconds, FPlatformTime::ToSeconds64(LastCamPosesTimes[bestIdx]));
	CameraCachePose = LastCamPoses[bestIdx];
	CameraPoseIndex = bestIdx;
}

bool UQRCodeDetectionComp::GetNearestPointBetweenLines(FVector P1, FVector Dir1, FVector P2, FVector Dir2, FVector& NearesPoint)
{
	NearesPoint = FVector::ZeroVector;
	Dir1.Normalize();
	Dir2.Normalize();
	FVector NormalPlane = FVector::CrossProduct(Dir1, Dir2);

	FVector U = FVector::DotProduct(P1 - P2, NormalPlane)* NormalPlane + P2;

	FVector B = FVector::DotProduct(U - P1, Dir2)* Dir2 + P1;

	FVector AuxiliarNormalPlane = B - U;
	AuxiliarNormalPlane.Normalize();

	FPlane APlane(U, AuxiliarNormalPlane);
	FVector RayDir = Dir1;

	// Check ray is not parallel to plane
	if ((RayDir | APlane) == 0.0f)
	{
		NearesPoint = FVector::ZeroVector;
		return false;
	}

	float T = ((APlane.W - (P1 | APlane)) / (RayDir | APlane));

	if (T < 0.f)
		return false;

	// Calculate intersection point
	NearesPoint = P1 + RayDir * T;

	return true;
}


void UQRCodeDetectionComp::GetLeftEyeWorldPos(FTransform HDM, FRotator& CamRot, FVector& CamLoc)
{
	if (GEngine && GEngine->StereoRenderingDevice.IsValid())
	{
		CamRot = HDM.GetRotation().Rotator();
		CamLoc = HDM.GetLocation();
		const float W2M = GetWorld()->GetWorldSettings()->WorldToMeters;
		GEngine->StereoRenderingDevice->CalculateStereoViewOffset(EStereoscopicEye::eSSE_LEFT_EYE, CamRot, W2M, CamLoc);
	}
}