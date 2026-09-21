#pragma once

#include "CoreMinimal.h"
#include "ItemData.h"

class UTexture2D;
class UStaticMesh;

// 장비 하나를 게임에서 다루는 데 필요한 정보를 한데 모은 것.
// DataTable 행(FItemData)에 장비 필드가 채워져 있으면 그 값에서, 아니면 EquipmentData.cpp의 코드 정의에서 만들어진다
struct FEquipInfo
{
	EEquipSlot Slot = EEquipSlot::None;

	FText Name;
	FText Description;

	int32 AttackBonus = 0;
	int32 MaxHpBonus = 0;

	// 플레이어 메시에 처음부터 붙어 있는(스킨에 구워진) 검/방패. 별도 모델을 만들지 않고 해당 본을 보이게/숨기는 것으로 표현한다
	bool bBuiltIn = false;

	// 별도 모델을 손에 붙이는 장비용. bBuiltIn이면 쓰이지 않는다
	TSoftObjectPtr<UStaticMesh> Mesh;
	FName Socket = NAME_None;
	FTransform Offset = FTransform::Identity;
};

namespace EquipDefs
{
	// 게임을 시작할 때 기본으로 들고 있는 검과 방패의 ItemID
	extern const FName GreystoneSword;
	extern const FName GreystoneShield;

	// 고블린이 쓰던 검. 고블린 토벌 퀘스트 보상 (DT_Item에 행이 이미 있다)
	extern const FName GoblinSword;

	// 코드에 정의된 장비. DataTable에 행이 없어도 인벤토리에 넣고 장착할 수 있다
	const FEquipInfo* FindDefinition(FName ItemID);

	// 시작 시 이 칸에 채워지는 아이템 (없으면 NAME_None)
	FName GetDefaultItem(EEquipSlot Slot);

	// 이 칸의 기본 장비 모델을 이루는 본. 칸이 기본 장비가 아닌 것으로 채워지거나 비면 이 본들을 숨긴다
	const TArray<FName>& GetBuiltInBones(EEquipSlot Slot);

	FText GetSlotLabel(EEquipSlot Slot);

	// DataTable에 아이콘이 없는 아이템을 위해 코드로 그려 만든 아이콘. 장비(검/방패)면 그 아이콘을, 아니면 코드로 정의된 전리품(LootData.h)의 아이콘을 돌려준다.
	// 아이콘을 찾는 쪽이 이 함수 하나만 부르면 되고, 어느 쪽도 아니면 nullptr. 한 번 만들고 재사용한다
	UTexture2D* GetGeneratedIcon(FName ItemID);
}
