#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_InventorySlot.generated.h"

UCLASS()
class CAPSTONE_API UUI_InventorySlot : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void InitSlot(FName InItemID);
	void ClearSlot();

protected:
	UPROPERTY(meta = (BindWidget))
	class UImage* ItemIcon;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FName CurrentItemID;
};
