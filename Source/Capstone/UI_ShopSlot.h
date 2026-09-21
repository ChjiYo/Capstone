#pragma once

#include "GameInfo.h"
#include "ItemData.h"
#include "UI_PanelParts.h"
#include "Blueprint/UserWidget.h"
#include "UI_ShopSlot.generated.h"

UENUM(BlueprintType)
enum class EShopSlotMode : uint8
{
    Buy,
    Sell
};

// 이번 클릭으로 거래가 안 되는 이유. None이면 거래할 수 있다
enum class EShopBlock : uint8
{
    None,
    NoGold,     // 골드가 모자란다
    NoRoom,     // 인벤토리에 넣을 자리가 없다
    NoStock     // 팔 물건이 없다
};

// 상점 창이 한 줄마다 계산해서 넘겨주는 내용 (선택한 수량, 그 값, 안 되는 이유)
struct FShopRowInfo
{
    int32 Quantity = 1;
    int32 TotalPrice = 0;
    int32 OwnedCount = 0;
    EShopBlock Block = EShopBlock::None;
};

// 상점 목록의 한 줄. 줄 전체가 버튼이라서 어디를 눌러도 거래를 요청한다 (실제 거래는 상점 창이 처리).
// bUseCodeBuiltLayout가 켜져 있으면(기본) 위젯 블루프린트(WBP_ShopSlot)의 트리를 버리고
// 아이콘 칸 + 이름/설명 + 값/보유 수 + 구매·판매 표시를 코드로 조립한다 (인벤토리/장비창과 같은 팔레트).
// 이때도 ItemIcon/NameText/PriceText/CountText/ActionButton 바인딩은 채워지므로 WBP를 그대로 쓰던 코드는 바뀐 게 없다
UCLASS()
class CAPSTONE_API UUI_ShopSlot : public UUserWidget
{
	GENERATED_BODY()

public:
    // 한 줄의 높이. 상점 창이 보이는 줄 수로 목록 높이를 정할 때 쓴다
    static constexpr float Height = 62.f;

    void InitSlot(FName InItemID, const FItemData* ItemData, EShopSlotMode InMode, const FShopRowInfo& Info);

    FName GetItemID() const { return CurrentItemID; }
    EShopSlotMode GetMode() const { return CurrentMode; }

    // 줄을 눌렀을 때 방송. 상점 창이 받아서 거래한다
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnTradeRequested, UUI_ShopSlot*);
    FOnTradeRequested OnTradeRequested;

protected:
    virtual void NativeOnInitialized() override;

    UFUNCTION()
    void OnActionButtonClicked();

    UPROPERTY(meta = (BindWidgetOptional))
    class UImage* ItemIcon;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* NameText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* PriceText;

    // 보유 수량 ("보유 3")
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* CountText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UButton* ActionButton;

    // 켜면 WBP 트리 대신 코드로 조립한 줄을 쓴다
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Layout")
    bool bUseCodeBuiltLayout = true;

    UPROPERTY(BlueprintReadOnly, Category = "Shop")
    FName CurrentItemID;

    EShopSlotMode CurrentMode = EShopSlotMode::Buy;

private:
    void BuildNativeTree();

    PanelUI::FIconBox IconBox;

    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> SubText;

    UPROPERTY(Transient)
    TObjectPtr<class UBorder> ActionPill;

    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> ActionLabel;

    bool bBuiltNative = false;
};
