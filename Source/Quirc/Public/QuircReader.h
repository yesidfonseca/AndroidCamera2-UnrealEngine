#pragma once
#include "CoreMinimal.h"

USTRUCT(BlueprintType)
struct FQRDetection
{
	FString Text;
	TArray<FVector2f> Corners; // usualmente 4 puntos
};

/** Lector QR basado en quirc (C puro) — no hereda de UObject */
class QUIRC_API FQuircReader
{
public:
	/**
	 * Decodifica QR a partir de un plano Luma (Y) 8-bit.
	 * @param Luma   Puntero al buffer Y (no nulo)
	 * @param Width  Ancho en píxeles
	 * @param Height Alto en píxeles
	 * @param Stride Bytes por fila en Luma (>= Width)
	 * @param Out    Resultados (limpia y rellena)
	 * @return true si encontró al menos un QR válido.
	 */
	static bool DecodeFromLuma(const uint8* Luma, int32 Width, int32 Height, int32 Stride,
	                           TArray<FQRDetection>& Out) noexcept;
};