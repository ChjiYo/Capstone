#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_Inventory.generated.h"

// 'I' 키로 여는 인벤토리 창.
// bUseCodeBuiltLayout가 켜져 있으면(기본) 위젯 블루프린트의 트리를 버리고 NativeOnInitialized에서
// RPG풍 패널(골드 이중 테두리 + 5x4 칸 격자 + 아이템 설명 칸 + 소지금)을 코드로 조립한다 (장비창과 같은 팔레트).
// 이때도 아래 바인딩 이름(CloseButton, InventoryWrapBox, GoldText)은 그대로 채워지므로 Py_Grey 쪽 코드는 바뀐 게 없다.
// 위젯 디자이너에서 직접 만든 레이아웃을 쓰고 싶으면 WBP의 클래스 기본값에서 bUseCodeBuiltLayout를 끈다
UCLASS()
class CAPSTONE_API UUI_Inventory : public UUserWidget
{
    GENERATED_BODY()

public:
    // 인벤토리 데이터를 받아 UI를 갱신하는 함수
    void RefreshInventoryUI(class UInventoryComponent* InInventoryComp);

    UFUNCTION()
    void OnCloseButtonClicked();

    const bool& GetOpenUI() const { return bOpenUI; }

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    // 장비창의 칸을 끌어다 이 창 위에 놓으면 그 장비를 벗어 인벤토리로 돌려보낸다
    virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

    // InventoryComponent::OnInventoryChanged 구독 콜백 — 포션 사용 등으로 인벤토리가 바뀔 때마다 자동 새로고침
    UFUNCTION()
    void HandleInventoryChanged();

    // 닫기 버튼
    UPROPERTY(meta = (BindWidget))
    class UButton* CloseButton;

    UPROPERTY(meta = (BindWidget))
    class UWrapBox* InventoryWrapBox;

    // 인벤토리 하단에 표시할 현재 골드 텍스트. BindWidgetOptional이라 위젯이 없어도 컴파일은 깨지지 않음
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* GoldText;

    // 비워 두면 UUI_InventorySlot(코드 조립 칸)을 쓴다
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUI_InventorySlot> InventorySlotClass;

    // 켜면 WBP 트리 대신 코드로 조립한 레이아웃을 쓴다
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Layout")
    bool bUseCodeBuiltLayout = true;

    UPROPERTY()
    TArray<UUI_InventorySlot*> SlotArray;

    UPROPERTY()
    class UInventoryComponent* InventoryComp;

protected:
    int32 MaxInventorySlots = 20;
    bool bOpenUI = false;

private:
    void BuildNativeTree();
    void CreateSlots();

    // 칸에 마우스가 들어오고 나갈 때. 아래쪽 설명 칸이 그 칸의 아이템을 보여준다
    void HandleSlotHover(class UUI_InventorySlot* HoveredWidget, bool bEntered);
    void RefreshInfoPanel();

    // 아래쪽 설명 칸 (코드 조립 레이아웃에서만 채워진다)
    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> InfoNameText;

    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> InfoTypeText;

    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> InfoBodyText;

    // "소지품 5 / 20"
    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> CapacityText;

    TWeakObjectPtr<class UUI_InventorySlot> HoveredSlot;
};
