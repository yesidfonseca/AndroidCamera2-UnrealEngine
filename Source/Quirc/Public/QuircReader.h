// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 Yesid Fonseca
#pragma once

#include "CoreMinimal.h"
#include "QuircReader.generated.h" 

USTRUCT(BlueprintType)
struct FQRDetection
{
public:
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quirc QRCode Text")
	FString Text;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quirc QRCode Corners")
	TArray<FVector2D> Corners; 
};


class QUIRC_API FQuircReader
{
public:
	/**
	 * Decodifica QR a partir de un plano Luma (Y) 8-bit o gray scale.
	 * @param Luma   Puntero al buffer Y (no nulo)
	 * @param Width  Ancho en píxeles
	 * @param Height Alto en píxeles
	 * @param Stride Bytes por fila en Luma (>= Width)
	 * @param Out    Resultados (limpia y rellena)
	 * @return true si encontró al menos un QR válido.
	 */
	static bool DecodeFromLuma(const uint8* Luma, int32 Width, int32 Height, int32 Stride,
	                           TArray<FQRDetection>& Out) ;
};