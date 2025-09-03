// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CamMotionAndQRDetectionComponent.generated.h"

UENUM(BlueprintType)
enum class EMotionKind : uint8
{
	None            UMETA(DisplayName = "None"),
	LateralLeft     UMETA(DisplayName = "Lateral Left"),
	LateralRight    UMETA(DisplayName = "Lateral Right"),
	VerticalUp		UMETA(DisplayName = "Vertical Up"),
	VerticalDown	UMETA(DisplayName = "Vertical Down"),
	Forward         UMETA(DisplayName = "Forward (approach)"),
	Backward        UMETA(DisplayName = "Backward (away)")
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CAM2ANDROID_API UCamMotionAndQRDetectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCamMotionAndQRDetectionComponent();

	UFUNCTION(BlueprintPure, Category = "CamMotion")
	EMotionKind GetMotionKind() const { return MotionKind; }

	UFUNCTION(BlueprintPure, Category = "CamMotion")
	void GetMotionMetrics(float& OutMeanUx, float& OutMeanUy, float& OutMeanMag, float& OutMeanRad, float& OutMeanTan) const
	{
		OutMeanUx = MeanUx; OutMeanUy = MeanUy; OutMeanMag = MeanMag; OutMeanRad = MeanRad; OutMeanTan = MeanTan;
	}

	/** CamMotion Settings */
	UPROPERTY(EditAnywhere, Category = "CamMotion", meta = (ClampMin = "4", ClampMax = "64"))
	int32 FlowStep = 12;             // salto de grilla (px)

	UPROPERTY(EditAnywhere, Category = "CamMotion", meta = (ClampMin = "1", ClampMax = "6"))
	int32 WindowRadius = 3;          // radio ventana LK (7x7 por defecto)

	UPROPERTY(EditAnywhere, Category = "CamMotion", meta = (ClampMin = "0.01", ClampMax = "5.0"))
	float MoveThreshold = 0.25f;     // pix/frame para "hay movimiento"

	UPROPERTY(EditAnywhere, Category = "CamMotion", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float RadialMin = 0.10f;         // componente radial mínima

	UPROPERTY(EditAnywhere, Category = "CamMotion", meta = (ClampMin = "0.5", ClampMax = "4.0"))
	float TangentialBias = 1.5f;     // tangencial domina si > bias * |radial|

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
private:
	void InitializeLuma(int32 InWidth, int32 InHeight, bool bCreateDebugTextures = true);
	// Dimensiones (compactas)
	int32 Width = 0;
	int32 Height = 0;

	// Buffers CPU compactos W*H
	int64 LastFrameTimestamp = 0;
	TArray<uint8> YPrev;
	TArray<uint8> YCurr;

	// Estado de movimiento
	EMotionKind MotionKind = EMotionKind::None;
	float MeanUx = 0.f, MeanUy = 0.f, MeanMag = 0.f, MeanRad = 0.f, MeanTan = 0.f;

	// Flujo óptico + clasificación
	struct FFlowVec { FVector2f P; FVector2f V; };
	void ComputeFlowAndClassify(const uint8* Curr, const uint8* Prev);
	static void GradientsAt(const uint8* Curr, const uint8* Prev, int x, int y, int W, float& Ix, float& Iy, float& It);


	TArray<FQRDetection> QRDetections;
};
