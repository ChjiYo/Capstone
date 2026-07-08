#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_Inventory.generated.h"

UCLASS()
class CAPSTONE_API UUI_Inventory : public UUserWidget
{
    GENERATED_BODY()

public:
    // �κ��丮 �����͸� �޾� UI�� �����ϴ� �Լ�
    void RefreshInventoryUI(class UInventoryComponent* InventoryComp);

    UFUNCTION()
    void OnCloseButtonClicked();

    const bool& GetOpenUI() const { return bOpenUI; }

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    // �ݱ� ��ư
    UPROPERTY(meta = (BindWidget))
    class UButton* CloseButton;

    UPROPERTY(meta = (BindWidget))
    class UWrapBox* InventoryWrapBox;

    // 인벤토리 우측 하단에 표시할 현재 골드 텍스트. BindWidgetOptional이라 위젯이 없어도 컴파일은 깨지지 않음
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* GoldText;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUI_InventorySlot> InventorySlotClass;

    UPROPERTY()
    TArray<UUI_InventorySlot*> SlotArray;

protected:
    int32 MaxInventorySlots = 20;
    bool bOpenUI = false;

	
};
