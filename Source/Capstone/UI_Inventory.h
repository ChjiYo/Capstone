#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_Inventory.generated.h"

UCLASS()
class CAPSTONE_API UUI_Inventory : public UUserWidget
{
    GENERATED_BODY()

public:
    // 인벤토리 데이터를 받아 UI를 갱신하는 함수
    void RefreshInventoryUI(class UInventoryComponent* InventoryComp);

    UFUNCTION()
    void OnCloseButtonClicked();

    const bool& GetOpenUI() const { return bOpenUI; }

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    // 닫기 버튼
    UPROPERTY(meta = (BindWidget))
    class UButton* CloseButton;

    UPROPERTY(meta = (BindWidget))
    class UWrapBox* InventoryWrapBox;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUI_InventorySlot> InventorySlotClass;

    UPROPERTY()
    TArray<UUI_InventorySlot*> SlotArray;

protected:
    int32 MaxInventorySlots = 20;
    bool bOpenUI = false;

	
};
