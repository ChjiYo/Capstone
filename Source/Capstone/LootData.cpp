#include "LootData.h"
#include "IconPainter.h"
#include "Engine/Texture2D.h"

const FName LootDefs::GolemCore(TEXT("Loot_GolemCore"));
const FName LootDefs::BirdFeather(TEXT("Loot_BirdFeather"));
const FName LootDefs::LarvaeSlime(TEXT("Loot_LarvaeSlime"));

namespace
{
	struct FDefEntry
	{
		FName ItemID;
		FLootInfo Info;
	};

	FDefEntry MakeEntry(FName ItemID, const TCHAR* Name, const TCHAR* Description, int32 SellPrice)
	{
		FDefEntry Entry;
		Entry.ItemID = ItemID;
		Entry.Info.Name = FText::FromString(Name);
		Entry.Info.Description = FText::FromString(Description);
		Entry.Info.SellPrice = SellPrice;
		return Entry;
	}

	const TArray<FDefEntry>& Definitions()
	{
		static TArray<FDefEntry> Table;

		if (Table.Num() > 0)
			return Table;

		Table.Add(MakeEntry(LootDefs::GolemCore, TEXT("골렘의 핵"), TEXT("골렘의 몸속에서 붉게 타오르던 광석. 아직도 온기가 남아 있다."), 150));
		Table.Add(MakeEntry(LootDefs::BirdFeather, TEXT("새의 깃털"), TEXT("가볍고 윤기가 흐르는 검푸른 깃털."), 35));
		Table.Add(MakeEntry(LootDefs::LarvaeSlime, TEXT("애벌레 점액"), TEXT("끈적하게 늘어지는 초록빛 점액. 비린내가 난다."), 25));

		return Table;
	}
}

const FLootInfo* LootDefs::FindDefinition(FName ItemID)
{
	for (const FDefEntry& Entry : Definitions())
	{
		if (Entry.ItemID == ItemID)
			return &Entry.Info;
	}

	return nullptr;
}

// ---------------------------------------------------------------------------------------------------------
// 코드로 그리는 아이콘 (그리는 도구는 IconPainter)
// ---------------------------------------------------------------------------------------------------------
namespace
{
	using namespace IconPainter;

	// 골렘의 핵: 금이 간 바위 덩어리 한가운데 박힌 붉은 광석
	void BuildGolemCoreShapes(TArray<FShape>& Shapes)
	{
		const FColor Line(24, 20, 18);
		const FColor Rock(120, 113, 106);
		const FColor RockDark(84, 79, 76);
		const FColor RockLight(158, 150, 138);
		const FColor Socket(52, 40, 38);
		const FColor Glow(255, 140, 50);
		const FColor GlowDark(204, 82, 24);
		const FColor GlowLight(255, 220, 156);

		Shapes.Add(MakePoly({ FVector2D(-0.30f, 0.30f), FVector2D(0.04f, 0.41f), FVector2D(0.33f, 0.25f), FVector2D(0.41f, -0.06f),
			FVector2D(0.27f, -0.34f), FVector2D(-0.05f, -0.41f), FVector2D(-0.34f, -0.24f), FVector2D(-0.41f, 0.06f) }, Rock, Line, 0.02f));

		// 아래쪽 그늘과 위쪽 빛 받는 면
		Shapes.Add(MakePoly({ FVector2D(-0.41f, 0.06f), FVector2D(0.41f, -0.06f), FVector2D(0.27f, -0.34f), FVector2D(-0.05f, -0.41f), FVector2D(-0.34f, -0.24f) }, RockDark, Line, 0.f));
		Shapes.Add(MakePoly({ FVector2D(-0.30f, 0.30f), FVector2D(0.04f, 0.41f), FVector2D(0.02f, 0.31f), FVector2D(-0.22f, 0.22f) }, RockLight, Line, 0.f));

		// 바위의 금
		Shapes.Add(MakePoly({ FVector2D(-0.37f, -0.12f), FVector2D(-0.29f, -0.08f), FVector2D(-0.28f, -0.11f), FVector2D(-0.37f, -0.16f) }, Line, Line, 0.f));
		Shapes.Add(MakePoly({ FVector2D(0.30f, 0.14f), FVector2D(0.36f, 0.20f), FVector2D(0.34f, 0.22f), FVector2D(0.28f, 0.17f) }, Line, Line, 0.f));

		// 광석이 박힌 홈과 광석
		Shapes.Add(MakeCircle(0.f, 0.f, 0.26f, Socket, Line, 0.014f));
		Shapes.Add(MakePoly({ FVector2D(0.f, 0.23f), FVector2D(0.15f, 0.f), FVector2D(0.f, -0.23f), FVector2D(-0.15f, 0.f) }, Glow, Line, 0.012f));
		Shapes.Add(MakePoly({ FVector2D(0.f, 0.23f), FVector2D(0.f, -0.23f), FVector2D(-0.15f, 0.f) }, GlowDark, Line, 0.f));
		Shapes.Add(MakePoly({ FVector2D(0.02f, 0.16f), FVector2D(0.08f, 0.03f), FVector2D(0.02f, 0.03f) }, GlowLight, Line, 0.f));
	}

	// 새의 깃털: 검푸른 깃털 한 장이 오른쪽 위로 비스듬히 누워 있다
	void BuildBirdFeatherShapes(TArray<FShape>& Shapes)
	{
		const FColor Line(24, 20, 18);
		const FColor VaneDark(46, 56, 92);
		const FColor VaneLight(80, 100, 148);
		const FColor Teal(90, 196, 202);
		const FColor Spine(234, 224, 198);

		Shapes.Add(MakePoly({ FVector2D(0.f, -0.26f), FVector2D(-0.11f, -0.10f), FVector2D(-0.155f, 0.10f), FVector2D(-0.10f, 0.30f),
			FVector2D(0.f, 0.46f), FVector2D(0.10f, 0.30f), FVector2D(0.155f, 0.10f), FVector2D(0.11f, -0.10f) }, VaneDark, Line, 0.02f));

		Shapes.Add(MakePoly({ FVector2D(0.f, -0.26f), FVector2D(0.f, 0.46f), FVector2D(0.10f, 0.30f), FVector2D(0.155f, 0.10f), FVector2D(0.11f, -0.10f) }, VaneLight, Line, 0.f));

		// 깃털 결을 따라 나 있는 청록빛 줄
		for (int32 i = 0; i < 4; ++i)
		{
			const float Y = -0.12f + 0.13f * i;
			Shapes.Add(MakePoly({ FVector2D(0.012f, Y), FVector2D(0.095f, Y + 0.07f), FVector2D(0.095f, Y + 0.10f), FVector2D(0.012f, Y + 0.03f) }, Teal, Line, 0.f));
		}

		// 가운데 대(축)와 아래로 빠진 깃대
		Shapes.Add(MakeRect(0.f, 0.f, 0.028f, 0.80f, Spine, Line, 0.01f));

		Rotate(Shapes, -40.f);
	}

	// 애벌레 점액: 끈적하게 뭉쳐 반짝이는 초록 방울
	void BuildLarvaeSlimeShapes(TArray<FShape>& Shapes)
	{
		const FColor Line(28, 56, 24);
		const FColor Slime(122, 206, 72);
		const FColor SlimeDark(84, 162, 50);
		const FColor Shine(214, 250, 166);
		const FColor Bubble(172, 236, 112);

		const TArray<FVector2D> Body = {
			FVector2D(-0.34f, -0.20f), FVector2D(-0.28f, -0.33f), FVector2D(-0.12f, -0.40f), FVector2D(0.12f, -0.40f), FVector2D(0.28f, -0.33f),
			FVector2D(0.34f, -0.20f), FVector2D(0.31f, 0.f), FVector2D(0.20f, 0.14f), FVector2D(0.10f, 0.28f), FVector2D(0.f, 0.40f),
			FVector2D(-0.10f, 0.28f), FVector2D(-0.20f, 0.14f), FVector2D(-0.31f, 0.f) };

		Shapes.Add(MakePoly(Body, Slime, Line, 0.02f));

		// 아래쪽 그늘
		Shapes.Add(MakePoly({ FVector2D(-0.34f, -0.20f), FVector2D(-0.28f, -0.33f), FVector2D(-0.12f, -0.40f), FVector2D(0.12f, -0.40f), FVector2D(0.28f, -0.33f),
			FVector2D(0.34f, -0.20f), FVector2D(0.30f, -0.10f), FVector2D(0.10f, -0.16f), FVector2D(-0.10f, -0.16f), FVector2D(-0.30f, -0.10f) }, SlimeDark, Line, 0.f));

		// 반짝임과 속에서 올라오는 거품
		Shapes.Add(MakeEllipse(-0.14f, 0.05f, 0.05f, 0.10f, Shine, Line, 0.f));
		Shapes.Add(MakeCircle(-0.06f, 0.21f, 0.022f, Shine, Line, 0.f));
		Shapes.Add(MakeCircle(0.13f, -0.04f, 0.05f, Bubble, Line, 0.012f));
		Shapes.Add(MakeCircle(0.05f, -0.24f, 0.032f, Bubble, Line, 0.01f));
		Shapes.Add(MakeCircle(-0.16f, -0.26f, 0.024f, Bubble, Line, 0.01f));
	}
}

UTexture2D* LootDefs::GetGeneratedIcon(FName ItemID)
{
	static TMap<FName, UTexture2D*> Cache;

	if (UTexture2D** Found = Cache.Find(ItemID))
		return *Found;

	TArray<IconPainter::FShape> Shapes;

	if (ItemID == GolemCore)
		BuildGolemCoreShapes(Shapes);
	else if (ItemID == BirdFeather)
		BuildBirdFeatherShapes(Shapes);
	else if (ItemID == LarvaeSlime)
		BuildLarvaeSlimeShapes(Shapes);
	else
		return nullptr;

	UTexture2D* Texture = IconPainter::Render(Shapes);

	Cache.Add(ItemID, Texture);
	return Texture;
}
