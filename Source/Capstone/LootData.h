#pragma once

#include "CoreMinimal.h"

class UTexture2D;

// 몬스터가 떨어뜨리는 전리품 하나의 정의.
// DT_Item(바이너리 에셋)에 행을 추가하지 않고도 아이템 이름/설명/판매가와 아이콘이 나오도록, 코드에 정의를 두고 인벤토리가 이 정보로 FItemData 행을 만들어 쓴다.
// DT_Item에 같은 ItemID의 행이 있으면 그 행이 우선한다 (에디터에서 값을 바꾸고 싶을 때)
struct FLootInfo
{
	FText Name;
	FText Description;

	// 상점 판매 탭에서 받는 값. 0이면 못 판다
	int32 SellPrice = 0;

	int32 MaxStack = 99;
};

namespace LootDefs
{
	extern const FName GolemCore;
	extern const FName BirdFeather;
	extern const FName LarvaeSlime;

	// 코드에 정의된 전리품. 없으면 nullptr
	const FLootInfo* FindDefinition(FName ItemID);

	// 코드로 그린 전리품 아이콘. 한 번 만들고 재사용한다. 정의가 없으면 nullptr
	UTexture2D* GetGeneratedIcon(FName ItemID);
}
