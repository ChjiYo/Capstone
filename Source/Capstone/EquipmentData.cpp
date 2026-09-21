#include "EquipmentData.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"

const FName EquipDefs::GreystoneSword(TEXT("GreystoneSword"));
const FName EquipDefs::GreystoneShield(TEXT("GreystoneShield"));

namespace
{
	struct FDefEntry
	{
		FName ItemID;
		FEquipInfo Info;
	};

	const TArray<FDefEntry>& Definitions()
	{
		static TArray<FDefEntry> Table;

		if (Table.Num() > 0)
			return Table;

		{
			FDefEntry Entry;
			Entry.ItemID = EquipDefs::GreystoneSword;
			Entry.Info.Slot = EEquipSlot::Weapon;
			Entry.Info.Name = FText::FromString(TEXT("그레이스톤 검"));
			Entry.Info.Description = FText::FromString(TEXT("가문에 대대로 전해 내려온 기사의 장검."));
			Entry.Info.AttackBonus = 5;
			Entry.Info.bBuiltIn = true;
			Table.Add(Entry);
		}

		{
			FDefEntry Entry;
			Entry.ItemID = EquipDefs::GreystoneShield;
			Entry.Info.Slot = EEquipSlot::Shield;
			Entry.Info.Name = FText::FromString(TEXT("그레이스톤 방패"));
			Entry.Info.Description = FText::FromString(TEXT("낡았지만 튼튼한 기사의 방패."));
			Entry.Info.MaxHpBonus = 40;
			Entry.Info.bBuiltIn = true;
			Table.Add(Entry);
		}

		// 고블린이 떨어뜨리는 소드. DT_Item에 GoblinSword 행이 이미 있어서 이름/아이콘/가격은 그쪽 값을 쓰고, 장비 수치와 모델만 여기서 채운다
		{
			FDefEntry Entry;
			Entry.ItemID = TEXT("GoblinSword");
			Entry.Info.Slot = EEquipSlot::Weapon;
			Entry.Info.Name = FText::FromString(TEXT("고블린 소드"));
			Entry.Info.Description = FText::FromString(TEXT("고블린이 쓰던 거친 칼. 투박하지만 묵직하다."));
			Entry.Info.AttackBonus = 8;
			Entry.Info.Mesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Goblin/Mesh/Weapons/SM_Weapons/SM_Sword.SM_Sword")));
			Entry.Info.Socket = TEXT("sword_bottom");
			// 손잡이가 본 원점에 오고 칼날이 +Z로 뻗는 메시라서, X축으로 35도 눕혀야 기본 검처럼 아래쪽으로 자연스럽게 늘어진다
			Entry.Info.Offset = FTransform(FRotator(0.f, 0.f, 35.f), FVector::ZeroVector, FVector(1.5f));
			Table.Add(Entry);
		}

		return Table;
	}
}

const FEquipInfo* EquipDefs::FindDefinition(FName ItemID)
{
	for (const FDefEntry& Entry : Definitions())
	{
		if (Entry.ItemID == ItemID)
			return &Entry.Info;
	}

	return nullptr;
}

FName EquipDefs::GetDefaultItem(EEquipSlot Slot)
{
	switch (Slot)
	{
	case EEquipSlot::Weapon: return GreystoneSword;
	case EEquipSlot::Shield: return GreystoneShield;
	default: return NAME_None;
	}
}

const TArray<FName>& EquipDefs::GetBuiltInBones(EEquipSlot Slot)
{
	static const TArray<FName> WeaponBones = { TEXT("sword_bottom"), TEXT("sword_top") };
	static const TArray<FName> ShieldBones = { TEXT("shield_inner"), TEXT("shield_outer") };
	static const TArray<FName> None;

	switch (Slot)
	{
	case EEquipSlot::Weapon: return WeaponBones;
	case EEquipSlot::Shield: return ShieldBones;
	default: return None;
	}
}

FText EquipDefs::GetSlotLabel(EEquipSlot Slot)
{
	switch (Slot)
	{
	case EEquipSlot::Weapon: return FText::FromString(TEXT("무기"));
	case EEquipSlot::Shield: return FText::FromString(TEXT("방패"));
	default: return FText::GetEmpty();
	}
}

// ---------------------------------------------------------------------------------------------------------
// 코드로 그리는 아이콘. 다각형 여러 장을 안티에일리어싱해서 겹쳐 그린다 (좌표는 -0.5~0.5, y가 위쪽)
// ---------------------------------------------------------------------------------------------------------
namespace
{
	struct FIconShape
	{
		TArray<FVector2D> Points;
		FColor Fill = FColor::White;
		FColor Outline = FColor::Black;
		float OutlineWidth = 0.f;
	};

	FIconShape MakeRect(float CX, float CY, float W, float H, FColor Fill, FColor Outline, float OutlineWidth)
	{
		FIconShape Shape;
		Shape.Points = { FVector2D(CX - W * 0.5f, CY - H * 0.5f), FVector2D(CX + W * 0.5f, CY - H * 0.5f), FVector2D(CX + W * 0.5f, CY + H * 0.5f), FVector2D(CX - W * 0.5f, CY + H * 0.5f) };
		Shape.Fill = Fill;
		Shape.Outline = Outline;
		Shape.OutlineWidth = OutlineWidth;
		return Shape;
	}

	FIconShape MakeCircle(float CX, float CY, float R, FColor Fill, FColor Outline, float OutlineWidth)
	{
		FIconShape Shape;
		const int32 Segments = 20;
		for (int32 i = 0; i < Segments; ++i)
		{
			const float Angle = 2.f * PI * i / Segments;
			Shape.Points.Add(FVector2D(CX + FMath::Cos(Angle) * R, CY + FMath::Sin(Angle) * R));
		}
		Shape.Fill = Fill;
		Shape.Outline = Outline;
		Shape.OutlineWidth = OutlineWidth;
		return Shape;
	}

	FIconShape MakePoly(const TArray<FVector2D>& Points, FColor Fill, FColor Outline, float OutlineWidth)
	{
		FIconShape Shape;
		Shape.Points = Points;
		Shape.Fill = Fill;
		Shape.Outline = Outline;
		Shape.OutlineWidth = OutlineWidth;
		return Shape;
	}

	void RotateShapes(TArray<FIconShape>& Shapes, float Degrees)
	{
		const float Rad = FMath::DegreesToRadians(Degrees);
		const float C = FMath::Cos(Rad);
		const float S = FMath::Sin(Rad);

		for (FIconShape& Shape : Shapes)
		{
			for (FVector2D& P : Shape.Points)
				P = FVector2D(P.X * C - P.Y * S, P.X * S + P.Y * C);
		}
	}

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

	void BuildSwordShapes(TArray<FIconShape>& Shapes, bool bRough)
	{
		const FColor Line(24, 20, 18);
		const FColor Blade = bRough ? FColor(158, 170, 128) : FColor(206, 216, 226);
		const FColor BladeDark = bRough ? FColor(112, 122, 88) : FColor(140, 156, 172);
		const FColor Guard = bRough ? FColor(150, 92, 52) : FColor(214, 172, 74);
		const FColor Grip = FColor(96, 62, 40);

		Shapes.Add(MakePoly({ FVector2D(-0.055f, -0.06f), FVector2D(0.055f, -0.06f), FVector2D(0.055f, 0.34f), FVector2D(0.f, 0.47f), FVector2D(-0.055f, 0.34f) }, Blade, Line, 0.016f));
		Shapes.Add(MakeRect(0.f, 0.20f, 0.024f, 0.40f, BladeDark, Line, 0.f));
		Shapes.Add(MakeRect(0.f, -0.085f, 0.36f, 0.055f, Guard, Line, 0.014f));
		Shapes.Add(MakeRect(0.f, -0.21f, 0.06f, 0.20f, Grip, Line, 0.014f));
		Shapes.Add(MakeCircle(0.f, -0.33f, 0.05f, Guard, Line, 0.014f));

		RotateShapes(Shapes, -45.f);
	}

	void BuildShieldShapes(TArray<FIconShape>& Shapes)
	{
		const FColor Line(24, 20, 18);
		const FColor Rim(206, 168, 78);
		const FColor Face(64, 104, 148);
		const FColor Stripe(220, 196, 130);

		const TArray<FVector2D> Outer = {
			FVector2D(-0.34f, 0.36f), FVector2D(0.34f, 0.36f), FVector2D(0.34f, 0.06f), FVector2D(0.28f, -0.10f),
			FVector2D(0.17f, -0.26f), FVector2D(0.f, -0.42f), FVector2D(-0.17f, -0.26f), FVector2D(-0.28f, -0.10f), FVector2D(-0.34f, 0.06f) };

		TArray<FVector2D> Inner;
		for (const FVector2D& P : Outer)
			Inner.Add(FVector2D(P.X * 0.78f, (P.Y - 0.02f) * 0.78f + 0.02f));

		Shapes.Add(MakePoly(Outer, Rim, Line, 0.018f));
		Shapes.Add(MakePoly(Inner, Face, Line, 0.012f));
		Shapes.Add(MakeRect(0.f, -0.02f, 0.07f, 0.56f, Stripe, Line, 0.f));
		Shapes.Add(MakeRect(0.f, 0.14f, 0.40f, 0.06f, Stripe, Line, 0.f));
		Shapes.Add(MakeCircle(0.f, 0.14f, 0.07f, Rim, Line, 0.012f));
	}
}

UTexture2D* EquipDefs::GetGeneratedIcon(FName ItemID)
{
	static TMap<FName, UTexture2D*> Cache;

	if (UTexture2D** Found = Cache.Find(ItemID))
		return *Found;

	const FEquipInfo* Info = FindDefinition(ItemID);

	if (!Info)
		return nullptr;

	TArray<FIconShape> Shapes;

	if (Info->Slot == EEquipSlot::Shield)
		BuildShieldShapes(Shapes);
	else
		BuildSwordShapes(Shapes, ItemID.ToString().Contains(TEXT("Goblin")));

	constexpr int32 Size = 96;
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

					for (const FIconShape& Shape : Shapes)
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

	Cache.Add(ItemID, Texture);
	return Texture;
}
