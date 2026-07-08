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
	UPROPERTY(meta = (BindWidget))
	class UImage* ItemIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* CountText;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FName CurrentItemID;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	int32 CurrentItemCount = 0;
};
