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
    void RefreshInventoryUI(class UInventoryComponent* InInventoryComp);

    UFUNCTION()
    void OnCloseButtonClicked();

    const bool& GetOpenUI() const { return bOpenUI; }

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    // 장비창의 칸을 끌어다 이 창 위에 놓으면 그 장비를 벗어 인벤토리로 돌려보낸다
    virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

    // InventoryComponent::OnInventoryChanged 구독 콜백 — 포션 사용 등으로 인벤토리가 바뀔 때마다 자동 새로고침
    UFUNCTION()
    void HandleInventoryChanged();

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

    UPROPERTY()
    class UInventoryComponent* InventoryComp;

protected:
    int32 MaxInventorySlots = 20;
    bool bOpenUI = false;

	
};
