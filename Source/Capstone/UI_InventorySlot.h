#pragma once

#include "GameInfo.h"
#include "InventoryComponent.h"
#include "Blueprint/UserWidget.h"
#include "UI_InventorySlot.generated.h"

UCLASS()
class CAPSTONE_API UUI_InventorySlot : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitSlot(const FInventorySlot& SlotData, const FItemData* ItemData);
	void ClearSlot();

protected:
	// 슬롯을 클릭하면 포션류 아이템을 즉시 사용(회복+소모)한다. 장비(검/방패)는 클릭하면 장착하고, 끌어서 장비창에 놓아도 장착한다
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

	// 이 슬롯의 아이템이 장비이면 true (플레이어의 장비 컴포넌트에 물어본다)
	bool IsEquippableItem() const;

	// 장비 슬롯을 누른 채 아직 끌기/클릭이 갈리지 않은 상태
	bool bPressedOnEquipment = false;

	UPROPERTY(meta = (BindWidget))
	class UImage* ItemIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* CountText;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FName CurrentItemID;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	int32 CurrentItemCount = 0;
};
