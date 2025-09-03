// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "QuircReader.h"
#include "QRCodeDetectionComp.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQRCodeDetected, TArray<FQRDetection>, QRCodesDetected);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
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


protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void InitializeLuma(int32 InWidth, int32 InHeight);
	// Dimensiones (compactas)
	int32 Width = 0;
	int32 Height = 0;

	// Buffer CPU compactos W*H
	int64 LastFrameTimestamp = 0;
	TArray<uint8> YCurr;

	TArray<FQRDetection> QRDetections;

};
