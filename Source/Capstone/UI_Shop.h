#pragma once

#include "GameInfo.h"
#include "UI_ShopSlot.h"
#include "Blueprint/UserWidget.h"
#include "UI_Shop.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopClosed);

// 한 번 클릭에 거래할 개수
UENUM()
enum class EShopQuantity : uint8
{
    One,    // 1개
    Ten,    // 10개
    Max     // 살 때는 골드/자리가 허락하는 최대, 팔 때는 가진 만큼 전부
};

// 상점 창. 위의 [구매][판매] 탭으로 목록을 바꾸고, 한 줄을 누르면 [수량] 칩이 정한 개수만큼 바로 거래한다.
// bUseCodeBuiltLayout가 켜져 있으면(기본) 위젯 블루프린트(WBP_Shop)의 트리를 버리고 골드 이중 테두리 판(인벤토리/장비창과 같은 모양)을
// 코드로 조립한다. 이때도 GoldText/CloseButton 바인딩은 채워지므로 NPC_Shop 쪽 코드는 바뀐 게 없다.
// 끄면 예전처럼 WBP의 BuyItemsPanel/SellItemsPanel에 줄을 채운다
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
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

    void RefreshShopUI();

    UFUNCTION()
    void HandleInventoryChanged();

    UFUNCTION()
    void OnCloseButtonClicked();

    UFUNCTION()
    void HandleBuyTab();

    UFUNCTION()
    void HandleSellTab();

    UFUNCTION()
    void HandleQuantityOne();

    UFUNCTION()
    void HandleQuantityTen();

    UFUNCTION()
    void HandleQuantityMax();

    UPROPERTY(meta = (BindWidgetOptional))
    class UPanelWidget* BuyItemsPanel;

    UPROPERTY(meta = (BindWidgetOptional))
    class UPanelWidget* SellItemsPanel;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* GoldText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UButton* CloseButton;

    // 비워 두면 UUI_ShopSlot(코드 조립 줄)을 쓴다
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUI_ShopSlot> ShopSlotClass;

    // 켜면 WBP 트리 대신 코드로 조립한 레이아웃을 쓴다
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Layout")
    bool bUseCodeBuiltLayout = true;

    UPROPERTY()
    TArray<FName> ShopItemIDs;

    UPROPERTY()
    class UInventoryComponent* InventoryComp;

private:
    void BuildNativeTree();

    // Panel을 비우고 Mode에 맞는 줄로 다시 채운다
    void PopulatePanel(class UPanelWidget* Panel, EShopSlotMode SlotMode);

    FShopRowInfo MakeRowInfo(FName ItemID, const FItemData& Data, EShopSlotMode SlotMode) const;

    void HandleTradeRequested(class UUI_ShopSlot* RowSlot);

    void SetMode(EShopSlotMode NewMode);
    void SetQuantity(EShopQuantity NewQuantity);

    // 창 아래쪽 한 줄 메시지 (거래 결과/실패 이유). 몇 초 뒤 서서히 사라지고 안내 문구로 돌아간다
    void ShowToast(const FString& Message, bool bGood);
    void ShowHint();

    // 한 번에 거래할 수 있는 최대 개수
    static constexpr int32 MaxPerTrade = 99;

    EShopSlotMode Mode = EShopSlotMode::Buy;
    EShopQuantity Quantity = EShopQuantity::One;

    // 인벤토리 변경은 방송이 잦아서(골드 차감, 아이템 추가가 따로 온다) 다음 틱에 한 번만 다시 그린다.
    // 줄의 클릭 콜백 안에서 그 줄을 없애는 일도 이렇게 피한다
    bool bRefreshPending = false;

    // 창이 뜬 직후 몇 틱 동안 키보드 포커스를 가져온다. 입력 모드를 바꾸는 순간 포커스가 게임 뷰포트로 가버려서 Esc가 안 먹기 때문
    int32 FocusTriesLeft = 0;

    float ToastRemaining = 0.f;

    // ---- 코드 조립 레이아웃에서만 채워진다 ----
    UPROPERTY(Transient)
    TObjectPtr<class UVerticalBox> ItemList;

    UPROPERTY(Transient)
    TObjectPtr<class UScrollBox> ListScroll;

    UPROPERTY(Transient)
    TObjectPtr<class UButton> BuyTabButton;

    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> BuyTabLabel;

    UPROPERTY(Transient)
    TObjectPtr<class UButton> SellTabButton;

    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> SellTabLabel;

    UPROPERTY(Transient)
    TArray<TObjectPtr<class UButton>> QuantityButtons;

    UPROPERTY(Transient)
    TArray<TObjectPtr<class UTextBlock>> QuantityLabels;

    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> ToastText;

    // "소지품 5 / 20"
    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> CapacityText;
};
