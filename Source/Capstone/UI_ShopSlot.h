#pragma once

#include "GameInfo.h"
#include "ItemData.h"
#include "Blueprint/UserWidget.h"
#include "UI_ShopSlot.generated.h"

UENUM(BlueprintType)
enum class EShopSlotMode : uint8
{
    Buy,
    Sell
};

UCLASS()
class CAPSTONE_API UUI_ShopSlot : public UUserWidget
{
	GENERATED_BODY()

public:
    void InitSlot(FName InItemID, const FItemData* ItemData, int32 OwnedCount, EShopSlotMode InMode, class UInventoryComponent* InInventoryComp);

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    class UImage* ItemIcon;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* NameText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* PriceText;

    // 판매 모드에서 보유 수량 표시(구매 모드에서는 사용 안 함)
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* CountText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UButton* ActionButton;

    UFUNCTION()
    void OnActionButtonClicked();

    UPROPERTY(BlueprintReadOnly, Category = "Shop")
    FName CurrentItemID;

    EShopSlotMode CurrentMode = EShopSlotMode::Buy;
    int32 CurrentPrice = 0;

    UPROPERTY()
    class UInventoryComponent* InventoryComp;
};
