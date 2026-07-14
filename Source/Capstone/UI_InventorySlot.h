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
	// 슬롯을 클릭하면 포션류 아이템을 즉시 사용(회복+소모)한다. 포션이 아니면 아무 일도 일어나지 않는다
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidget))
	class UImage* ItemIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* CountText;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FName CurrentItemID;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	int32 CurrentItemCount = 0;
};
