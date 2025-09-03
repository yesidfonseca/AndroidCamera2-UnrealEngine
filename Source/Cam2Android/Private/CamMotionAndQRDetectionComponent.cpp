// Fill out your copyright notice in the Description page of Project Settings.


#include "CamMotionAndQRDetectionComponent.h"
#include "AndroidCamera2Subsystem.h"
#include "Kismet/GameplayStatics.h" 
#include "Engine/Engine.h"

// Sets default values for this component's properties
UCamMotionAndQRDetectionComponent::UCamMotionAndQRDetectionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

}


// Called when the game starts
void UCamMotionAndQRDetectionComponent::BeginPlay()
{
	Super::BeginPlay();

}


// Called every frame
void UCamMotionAndQRDetectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
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
						InitializeLuma(Width, Height, /*bCreateDebugTextures*/ true);
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


	// primer frame -> solo inicializa
	static bool bPrimera = true;
	if (bPrimera)
	{
		YPrev = YCurr;
		bPrimera = false;
		return;
	}

	ComputeFlowAndClassify(YCurr.GetData(), YPrev.GetData());

	OnQRCodeDetected.Broadcast(QRDetections);
	OnCamMotionDected.Broadcast(MotionKind);

	// actualizar historia
	YPrev = YCurr;
}



void UCamMotionAndQRDetectionComponent::InitializeLuma(int32 InWidth, int32 InHeight, bool bCreateDebugTextures)
{
	check(InWidth > 0 && InHeight > 0);
	Width = InWidth;
	Height = InHeight;

	YPrev.SetNumZeroed(Width * Height);
	YCurr.SetNumZeroed(Width * Height);

	MotionKind = EMotionKind::None;
	MeanUx = MeanUy = MeanMag = MeanRad = MeanTan = 0.f;
}

void UCamMotionAndQRDetectionComponent::GradientsAt(const uint8* Curr, const uint8* Prev, int x, int y, int W, float& Ix, float& Iy, float& It)
{
	// Sobel 3x3 simple en Curr, diferencia temporal vs Prev
	auto PX = [&](const uint8* Img, int X, int Y) -> uint8 { return Img[Y * W + X]; };

	const int xm1 = x - 1, xp1 = x + 1, ym1 = y - 1, yp1 = y + 1;

	const float gx =
		-PX(Curr, xm1, ym1) - 2 * PX(Curr, xm1, y) - PX(Curr, xm1, yp1) +
		PX(Curr, xp1, ym1) + 2 * PX(Curr, xp1, y) + PX(Curr, xp1, yp1);

	const float gy =
		-PX(Curr, xm1, ym1) - 2 * PX(Curr, x, ym1) - PX(Curr, xp1, ym1) +
		PX(Curr, xm1, yp1) + 2 * PX(Curr, x, yp1) + PX(Curr, xp1, yp1);

	Ix = gx * (1.f / 8.f);
	Iy = gy * (1.f / 8.f);
	It = float(PX(Curr, x, y)) - float(PX(Prev, x, y));
}

void UCamMotionAndQRDetectionComponent::ComputeFlowAndClassify(const uint8* Curr, const uint8* Prev)
{
	check(Curr && Prev);
	const int32 Step = FMath::Clamp(FlowStep, 4, 64);
	const int32 Rad = FMath::Clamp(WindowRadius, 1, 6);
	const float Eps = 1e-4f;

	TArray<FFlowVec> Flow;
	Flow.Reserve(((Width / Step) + 1) * ((Height / Step) + 1));

	for (int y = Rad; y < Height - Rad; y += Step)
	{
		for (int x = Rad; x < Width - Rad; x += Step)
		{
			float Sxx = 0.f, Sxy = 0.f, Syy = 0.f, Sxt = 0.f, Syt = 0.f;

			for (int j = -Rad; j <= Rad; ++j)
			{
				for (int i = -Rad; i <= Rad; ++i)
				{
					float Ix, Iy, It;
					GradientsAt(Curr, Prev, x + i, y + j, Width, Ix, Iy, It);
					Sxx += Ix * Ix; Sxy += Ix * Iy; Syy += Iy * Iy;
					Sxt += Ix * It; Syt += Iy * It;
				}
			}

			const float det = (Sxx * Syy) - (Sxy * Sxy);
			if (det < Eps) continue;

			const float bx = -Sxt;
			const float by = -Syt;
			const float u = (bx * Syy - by * Sxy) / det;
			const float v = (by * Sxx - bx * Sxy) / det;

			if (FMath::IsFinite(u) && FMath::IsFinite(v) && (FMath::Abs(u) + FMath::Abs(v) < 50.f))
			{
				Flow.Add({ FVector2f((float)x, (float)y), FVector2f(u, v) });
			}
		}
	}

	// Calcular métricas y clasificar
	if (Flow.Num() == 0)
	{
		MotionKind = EMotionKind::None;
		MeanUx = MeanUy = MeanMag = MeanRad = MeanTan = 0.f;
		return;
	}

	const FVector2f C(Width * 0.5f, Height * 0.5f);
	double Sux = 0, Suy = 0, Smag = 0, Srad = 0, Stan = 0;
	const double N = (double)Flow.Num();

	for (const auto& F : Flow)
	{
		const FVector2f v = F.V;
		Sux += v.X; Suy += v.Y; Smag += v.Size();

		FVector2f r = F.P - C;
		const float rlen = FMath::Max(1e-3f, r.Size());
		const FVector2f rhat = r / rlen;
		const FVector2f that(-rhat.Y, rhat.X);

		const float vr = FVector2f::DotProduct(v, rhat);
		const float vt = FVector2f::DotProduct(v, that);

		Srad += vr;
		Stan += FMath::Abs(vt);
	}

	MeanUx = (float)(Sux / N);
	MeanUy = (float)(Suy / N);
	MeanMag = (float)(Smag / N);
	MeanRad = (float)(Srad / N);
	MeanTan = (float)(Stan / N);

	// Umbrales
	const float kMove = MoveThreshold;
	const float kRadMin = RadialMin;
	const float kTanBias = TangentialBias;

	if (MeanMag < kMove)
	{
		MotionKind = EMotionKind::None;
		return;
	}

	// ¿Predomina tangencial? => lateral (signo de U promedio)
	if (MeanTan > kTanBias * FMath::Abs(MeanRad))
	{
		if (FMath::Abs(MeanUx) > FMath::Abs(MeanUy))
		{
			MotionKind = (MeanUx < 0.f) ? EMotionKind::LateralLeft : EMotionKind::LateralRight;
		}
		else
		{
			MotionKind = (MeanUy < 0.f) ? EMotionKind::VerticalDown : EMotionKind::VerticalUp;
		}
		return;
	}

	// Radial: expansión (Forward) / contracción (Backward)
	if (MeanRad > kRadMin) { MotionKind = EMotionKind::Forward;  return; }
	if (MeanRad < -kRadMin) { MotionKind = EMotionKind::Backward; return; }

	MotionKind = EMotionKind::None;
}

