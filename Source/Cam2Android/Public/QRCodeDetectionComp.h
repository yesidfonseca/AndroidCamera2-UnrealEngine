// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 Yesid Fonseca

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "QuircReader.h"
#include "QRCodeDetectionComp.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQRCodeDetected, TArray<FQRDetection>, QRCodesDetected);

USTRUCT(BlueprintType)
struct FTriangulationSolver
{
	GENERATED_BODY();

public:

	
	float Step;
	FVector Delta;
	FVector EstimatedPoint = FVector(1);
	int32 LineMaxSamples = 5;

	UPROPERTY(BlueprintReadOnly)	
	TArray<FVector> LinesPoint;

	UPROPERTY(BlueprintReadOnly)
	TArray<FVector> LinesDirection;

	bool AddLine(FVector LinePoint, FVector LineDirection, bool bForceAdd = false)
	{
		if (LinesDirection.Num() < LineMaxSamples)
		{
			LineDirection.Normalize();
			LinesDirection.Add(LineDirection);
			LinesPoint.Add(LinePoint);
			LineSampleIdx = LinesDirection.Num()-1;
		}
		else
		{
			int32 PosibleLineSampleIdx = (LineSampleIdx + 1) % LineMaxSamples;
			LineDirection.Normalize();
			 
			if (!bForceAdd)
			{
				float CurrentSumAngles = 0.f;
				float PosibleSumAngles = 0.f;
				for (int32 i = 0; i < LineMaxSamples; i++)
				{
					if (i != PosibleLineSampleIdx)
					{
						CurrentSumAngles += FMath::Acos(FVector::DotProduct(LinesDirection[i], LinesDirection[PosibleLineSampleIdx]));
						PosibleSumAngles += FMath::Acos(FVector::DotProduct(LinesDirection[i], LineDirection));
					}
				}

				if (PosibleSumAngles*0.9 <= CurrentSumAngles || FVector::Distance(LinesPoint[PosibleLineSampleIdx], LinePoint)<2.f)
				{
					return false;
				}
				UE_LOG(LogTemp, Display, TEXT("FTriangulationSolver::AddLine. PosibleSumAngles:%f, CurrentSumAngles: %f"), PosibleSumAngles, CurrentSumAngles);
			}

			LineSampleIdx = PosibleLineSampleIdx;
			LinesDirection[LineSampleIdx] = LineDirection;
			LinesPoint[LineSampleIdx] = LinePoint;
		}

		HessianMatrix = {};
		for (int32 i = 0; i < LinesDirection.Num(); i++)
		{
			float x = LinesDirection[i].X, y = LinesDirection[i].Y, z = LinesDirection[i].Z;

			HessianMatrix[0][0] -= x * x - 1.f;	HessianMatrix[0][1] -= x * y;		HessianMatrix[0][2] -= x * z;
			HessianMatrix[1][0] -= y * x;		HessianMatrix[1][1] -= y * y - 1.f;	HessianMatrix[1][2] -= y * z;
			HessianMatrix[2][0] -= z * x;		HessianMatrix[2][1] -= z * y;		HessianMatrix[2][2] -= z * z - 1.f;
		}

		int32 Precision = 3;
		FString HessianString = FString::Printf(
			TEXT("[[%.*f,%.*f,%.*f],[%.*f,%.*f,%.*f],[%.*f,%.*f,%.*f]]"),
			Precision, HessianMatrix[0][0], Precision, HessianMatrix[0][1], Precision, HessianMatrix[0][2],
			Precision, HessianMatrix[1][0], Precision, HessianMatrix[1][1], Precision, HessianMatrix[1][2],
			Precision, HessianMatrix[2][0], Precision, HessianMatrix[2][1], Precision, HessianMatrix[2][2]
		);

		UE_LOG(LogTemp, Display, TEXT("FTriangulationSolver::AddLine. HessianMatrix: %s"), *HessianString);

		return true;
	}

	FVector ComputeEstimatedPoint(float ModifierStep = 1.f)
	{
		ModifierStep = FMath::Clamp(ModifierStep, 0.001f, 1.f);
		ComputeGradient();
		EstimatedPoint = EstimatedPoint - FMath::Max(ModifierStep * Step, 0.1f) * Delta;
		
		return EstimatedPoint;
	}

	void Reset()
	{
		LinesDirection.Empty();
		LinesPoint.Empty();
	}

private:
	
	
	int32 LineSampleIdx = 0;

	std::array<std::array<double, 3>, 3> HessianMatrix;
	std::array<std::array<double, 3>, 3> HessianMatrix_Inv;
	
	void ComputeGradient()
	{
		Delta = FVector::ZeroVector;
		for (int32 i = 0; i < LinesDirection.Num(); i++)
		{
			FVector w = EstimatedPoint - LinesPoint[i];
			float dot = FVector::DotProduct(w, LinesDirection[i]);
			FVector proj = dot * LinesDirection[i];
			FVector perp = w - proj;
			Delta += perp;
		}

		const float x = Delta.X, y = Delta.Y, z = Delta.Z;
		FVector MV = FVector(
			HessianMatrix[0][0] * x + HessianMatrix[0][1] * y + HessianMatrix[0][2] * z,
			HessianMatrix[1][0] * x + HessianMatrix[1][1] * y + HessianMatrix[1][2] * z,
			HessianMatrix[2][0] * x + HessianMatrix[2][1] * y + HessianMatrix[2][2] * z
		);

		Step = 2.f *( x * MV.X + y* MV.Y + z * MV.Z);
		if(Step>0.000001f)
			Step = FVector::DotProduct(Delta , Delta) / Step;

	}
	
	bool InverseSymmetric_Adjugate(double eps = 1e-12)
	{
		const double a = HessianMatrix[0][0], b = HessianMatrix[0][1], c = HessianMatrix[0][2];
		const double d = HessianMatrix[1][1], e = HessianMatrix[1][2];
		const double f = HessianMatrix[2][2];

		const double C00 = d * f - e * e;
		const double C01 = c * e - b * f;
		const double C02 = b * e - c * d;
		const double C11 = a * f - c * c;
		const double C12 = b * c - a * e;
		const double C22 = a * d - b * b;

		const double det = a * C00 + b * C01 + c * C02;
		if (FMath::Abs(det) <= eps) return false;

		const double invDet = 1.0 / det;

		std::array<std::array<double, 3>, 3> HessianMatrix_Inv2 = { {  { C00 * invDet, C01 * invDet, C02 * invDet },
				   { C01 * invDet, C11 * invDet, C12 * invDet },
				   { C02 * invDet, C12 * invDet, C22 * invDet }  } };
		HessianMatrix_Inv = HessianMatrix_Inv2;
		return true;
	}

	
	
};

class UCameraComponent;

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CAM2ANDROID_API UQRCodeDetectionComp : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UQRCodeDetectionComp();

	UFUNCTION(BlueprintPure, Category = "Quirc QRCode")
	TArray<FQRDetection>  GetQRCodesDetected() const { return QRDetections; }

	UPROPERTY(BlueprintAssignable, Category = "Quirc QRCode")
	FOnQRCodeDetected OnQRCodeDetected;

	UPROPERTY(BlueprintReadWrite)
	UCameraComponent* Camera;

	
		

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadOnly)
	TArray<FVector> EstimatedCornersLocations;

	UPROPERTY(BlueprintReadOnly)
	TArray<FVector> LastQRCodeCornersRay;

	UPROPERTY(BlueprintReadOnly)
	FVector LastQRCodeStartPointRay;

	UFUNCTION(BlueprintNativeEvent)
	void OnQRUsedForPoseEstimation();

	UPROPERTY(BlueprintReadOnly)
	FTransform CameraCachePose;

	

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "2"))
	int32 QRCodeTriangulationSolverSamples = 3;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "10.0"))
	int32 CameraSamples = 30;

	UPROPERTY(BlueprintReadOnly)
	TArray<FTriangulationSolver> QRCodeCornerTSolvers;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	

private:
	void InitializeLuma(int32 InWidth, int32 InHeight);
	
	// Dimensiones (compactas)
	int32 Width = 0;
	int32 Height = 0;

	// Buffer CPU compactos W*H
	uint64 LastFrameTimestamp = 0;
	TArray<uint8> YCurr;

	TArray<FQRDetection> QRDetections;

	TArray<FTransform> LastCamPoses;

	TArray<uint64> LastCamPosesTimes;

	void UpdateCameraCachePose();

	bool GetNearestPointBetweenLines(FVector P1, FVector Dir1, FVector P2, FVector Dir2, FVector& NearesPoint);

	int32 CameraPoseIndex = 0;
	
	void GetEyeWorldPos(FString CameraId, FTransform HDM, FRotator& CamRot, FVector& CamLoc);

};
