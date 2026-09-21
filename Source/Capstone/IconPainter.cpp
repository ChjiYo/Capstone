#include "IconPainter.h"
#include "Engine/Texture2D.h"

namespace
{
	// 점에서 다각형 테두리까지의 거리. 다각형 안쪽이면 음수
	float SignedDistance(const TArray<FVector2D>& Points, const FVector2D& P)
	{
		float MinDistSq = FLT_MAX;
		bool bInside = false;

		for (int32 i = 0, j = Points.Num() - 1; i < Points.Num(); j = i++)
		{
			const FVector2D& A = Points[j];
			const FVector2D& B = Points[i];
			const FVector2D AB = B - A;
			const float T = FMath::Clamp(FVector2D::DotProduct(P - A, AB) / FMath::Max(AB.SizeSquared(), UE_SMALL_NUMBER), 0.f, 1.f);

			MinDistSq = FMath::Min(MinDistSq, (P - (A + AB * T)).SizeSquared());

			if (((A.Y > P.Y) != (B.Y > P.Y)) && (P.X < (B.X - A.X) * (P.Y - A.Y) / (B.Y - A.Y) + A.X))
				bInside = !bInside;
		}

		const float Distance = FMath::Sqrt(MinDistSq);
		return bInside ? -Distance : Distance;
	}
}

IconPainter::FShape IconPainter::MakeRect(float CX, float CY, float W, float H, FColor Fill, FColor Outline, float OutlineWidth)
{
	FShape Shape;
	Shape.Points = { FVector2D(CX - W * 0.5f, CY - H * 0.5f), FVector2D(CX + W * 0.5f, CY - H * 0.5f), FVector2D(CX + W * 0.5f, CY + H * 0.5f), FVector2D(CX - W * 0.5f, CY + H * 0.5f) };
	Shape.Fill = Fill;
	Shape.Outline = Outline;
	Shape.OutlineWidth = OutlineWidth;
	return Shape;
}

IconPainter::FShape IconPainter::MakeCircle(float CX, float CY, float R, FColor Fill, FColor Outline, float OutlineWidth)
{
	return MakeEllipse(CX, CY, R, R, Fill, Outline, OutlineWidth);
}

IconPainter::FShape IconPainter::MakeEllipse(float CX, float CY, float RX, float RY, FColor Fill, FColor Outline, float OutlineWidth)
{
	FShape Shape;
	const int32 Segments = 24;

	for (int32 i = 0; i < Segments; ++i)
	{
		const float Angle = 2.f * PI * i / Segments;
		Shape.Points.Add(FVector2D(CX + FMath::Cos(Angle) * RX, CY + FMath::Sin(Angle) * RY));
	}

	Shape.Fill = Fill;
	Shape.Outline = Outline;
	Shape.OutlineWidth = OutlineWidth;
	return Shape;
}

IconPainter::FShape IconPainter::MakePoly(const TArray<FVector2D>& Points, FColor Fill, FColor Outline, float OutlineWidth)
{
	FShape Shape;
	Shape.Points = Points;
	Shape.Fill = Fill;
	Shape.Outline = Outline;
	Shape.OutlineWidth = OutlineWidth;
	return Shape;
}

void IconPainter::Rotate(TArray<FShape>& Shapes, float Degrees)
{
	const float Rad = FMath::DegreesToRadians(Degrees);
	const float C = FMath::Cos(Rad);
	const float S = FMath::Sin(Rad);

	for (FShape& Shape : Shapes)
	{
		for (FVector2D& P : Shape.Points)
			P = FVector2D(P.X * C - P.Y * S, P.X * S + P.Y * C);
	}
}

UTexture2D* IconPainter::Render(const TArray<FShape>& Shapes, int32 Size)
{
	constexpr int32 Sub = 4;

	TArray<FColor> Pixels;
	Pixels.SetNumZeroed(Size * Size);

	for (int32 Y = 0; Y < Size; ++Y)
	{
		for (int32 X = 0; X < Size; ++X)
		{
			float SumR = 0.f, SumG = 0.f, SumB = 0.f, SumA = 0.f;

			for (int32 SY = 0; SY < Sub; ++SY)
			{
				for (int32 SX = 0; SX < Sub; ++SX)
				{
					const FVector2D P((X + (SX + 0.5f) / Sub) / Size - 0.5f, 0.5f - (Y + (SY + 0.5f) / Sub) / Size);

					const FColor* Hit = nullptr;

					for (const FShape& Shape : Shapes)
					{
						const float D = SignedDistance(Shape.Points, P);

						if (D < 0.f)
							Hit = &Shape.Fill;
						else if (D < Shape.OutlineWidth)
							Hit = &Shape.Outline;
					}

					if (Hit)
					{
						SumR += Hit->R;
						SumG += Hit->G;
						SumB += Hit->B;
						SumA += 1.f;
					}
				}
			}

			if (SumA > 0.f)
			{
				FColor& Out = Pixels[Y * Size + X];
				Out.R = FMath::RoundToInt(SumR / SumA);
				Out.G = FMath::RoundToInt(SumG / SumA);
				Out.B = FMath::RoundToInt(SumB / SumA);
				Out.A = FMath::RoundToInt(255.f * SumA / (Sub * Sub));
			}
		}
	}

	UTexture2D* Texture = UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8);

	if (!Texture)
		return nullptr;

	FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
	Mip.BulkData.Unlock();

	Texture->SRGB = true;
	Texture->NeverStream = true;
	Texture->UpdateResource();

	// 코드로 만든 텍스처는 어디서도 참조되지 않으므로, 가비지 컬렉션이 지워버리지 않게 루트에 붙여둔다
	Texture->AddToRoot();

	return Texture;
}
