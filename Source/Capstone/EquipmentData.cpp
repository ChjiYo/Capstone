#include "EquipmentData.h"
#include "IconPainter.h"
#include "LootData.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"

const FName EquipDefs::GreystoneSword(TEXT("GreystoneSword"));
const FName EquipDefs::GreystoneShield(TEXT("GreystoneShield"));
const FName EquipDefs::GoblinSword(TEXT("GoblinSword"));

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
			Entry.ItemID = EquipDefs::GoblinSword;
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
// 코드로 그리는 아이콘 (그리는 도구는 IconPainter)
// ---------------------------------------------------------------------------------------------------------
namespace
{
	using namespace IconPainter;

	void BuildSwordShapes(TArray<FShape>& Shapes, bool bRough)
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

		Rotate(Shapes, -45.f);
	}

	void BuildShieldShapes(TArray<FShape>& Shapes)
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

	// 장비가 아니면 코드로 정의된 전리품 아이콘이 있는지 본다 (아이콘을 찾는 쪽이 한 함수만 부르면 되도록)
	if (!Info)
		return LootDefs::GetGeneratedIcon(ItemID);

	TArray<IconPainter::FShape> Shapes;

	if (Info->Slot == EEquipSlot::Shield)
		BuildShieldShapes(Shapes);
	else
		BuildSwordShapes(Shapes, ItemID.ToString().Contains(TEXT("Goblin")));

	UTexture2D* Texture = IconPainter::Render(Shapes);

	Cache.Add(ItemID, Texture);
	return Texture;
}
