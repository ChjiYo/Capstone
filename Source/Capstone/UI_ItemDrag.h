#pragma once

#include "GameInfo.h"
#include "ItemData.h"
#include "Blueprint/DragDropOperation.h"
#include "UI_ItemDrag.generated.h"

// 인벤토리 슬롯 <-> 장비창 슬롯 사이로 아이템을 끌어다 놓을 때 마우스에 딸려오는 정보
UCLASS()
class CAPSTONE_API UItemDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName ItemID;

	// 장비창 칸에서 끌어온 것이면 그 칸. 인벤토리에서 끌어온 것이면 None
	UPROPERTY()
	EEquipSlot FromEquipSlot = EEquipSlot::None;
};

namespace ItemDrag
{
	// 마우스를 따라다니는 작은 아이콘 카드. 아이콘이 없으면 이름만 보여준다
	UWidget* MakeVisual(UObject* Outer, const FText& Name, class UTexture2D* Icon);
}
