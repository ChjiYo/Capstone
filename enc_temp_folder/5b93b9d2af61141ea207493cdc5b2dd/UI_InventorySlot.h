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
	void InitSlot(const FInventorySlot& SlotData);
	void ClearSlot();

protected:
	UPROPERTY(meta = (BindWidget))
	class UImage* ItemIcon;

	// 수량 표시 텍스트. WBP_InventorySlot에 없어도 컴파일은 깨지지 않음
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* CountText;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FName CurrentItemID;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	int32 CurrentItemCount = 0;
};
