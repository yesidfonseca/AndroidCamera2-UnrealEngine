#include "QuircReader.h"
#include <string>

extern "C" {
	#include "quirc.h" // asegúrate de tener el include path al folder donde está quirc.h
}

bool FQuircReader::DecodeFromLuma(const uint8* Luma, int32 W, int32 H, int32 Stride,
                                  TArray<FQRDetection>& Out) 
{
	Out.Reset();

	// Validaciones defensivas
	if (!Luma || W <= 0 || H <= 0 || Stride < W)
	{
		return false;
	}

	quirc* Q = quirc_new();
	if (!Q)
	{
		return false;
	}

	bool bAny = false;

	if (quirc_resize(Q, W, H) == 0)
	{
		int iW = 0, iH = 0;
		uint8_t* Img = quirc_begin(Q, &iW, &iH); // buffer interno W×H que debemos llenar
		if (Img && iW == W && iH == H)
		{
			// Copia fila a fila desde Luma (stride externo) al buffer interno (packed)
			for (int y = 0; y < H; ++y)
			{
				FMemory::Memcpy(Img + y * W, Luma + y * Stride, W);
			}

			quirc_end(Q);

			const int CodeCount = quirc_count(Q);
			for (int i = 0; i < CodeCount; ++i)
			{
				quirc_code Code;
				quirc_data Data;
				quirc_extract(Q, i, &Code);

				if (quirc_decode(&Code, &Data) == QUIRC_SUCCESS)
				{
					FQRDetection R;

					// Texto: Data.payload (UTF-8, length = payload_len; puede no estar null-terminated)
					const std::string S(reinterpret_cast<const char*>(Data.payload),
					                    static_cast<size_t>(Data.payload_len));
					R.Text = UTF8_TO_TCHAR(S.c_str());

					// Esquinas (siempre 4 en quirc), en orden TL, TR, BR, BL aprox.
					
					R.Corners.Reserve(4);
					for (int c = 0; c < 4; ++c)
					{
						R.Corners.Emplace(static_cast<float>(Code.corners[c].x),
						                  static_cast<float>(Code.corners[c].y));
					}

					Out.Add(MoveTemp(R));
					bAny = true;
				}
			}
		}
	}

	quirc_destroy(Q);
	return bAny;
}