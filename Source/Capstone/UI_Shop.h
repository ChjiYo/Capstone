#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_Shop.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopClosed);

UCLASS()
class CAPSTONE_API UUI_Shop : public UUserWidget
{
	GENERATED_BODY()

public:
    void InitShop(const TArray<FName>& InShopItemIDs, class UInventoryComponent* InInventoryComp);

    // 상점을 연 NPC가 바인딩해서 위젯 제거/입력모드 복구를 처리
    UPROPERTY(BlueprintAssignable, Category = "Shop")
    FOnShopClosed OnShopClosed;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    void RefreshShopUI();

    UFUNCTION()
    void HandleInventoryChanged();

    UFUNCTION()
    void OnCloseButtonClicked();

    UPROPERTY(meta = (BindWidgetOptional))
    class UPanelWidget* BuyItemsPanel;

    UPROPERTY(meta = (BindWidgetOptional))
    class UPanelWidget* SellItemsPanel;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* GoldText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UButton* CloseButton;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUI_ShopSlot> ShopSlotClass;

    UPROPERTY()
    TArray<FName> ShopItemIDs;

    UPROPERTY()
    class UInventoryComponent* InventoryComp;
};
