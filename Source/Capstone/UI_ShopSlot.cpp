#include "UI_ShopSlot.h"
#include "Components/ButtonSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UUI_ShopSlot::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // 위젯 블루프린트의 트리를 통째로 코드 조립 트리로 교체한다. 디자이너 미리보기가 아니라 실제 인스턴스에서만
    if (bUseCodeBuiltLayout && WidgetTree && !IsDesignTime())
    {
        BuildNativeTree();
    }

    // 버튼 연결은 한 번만 한다 (InitSlot은 목록을 새로 그릴 때마다 불린다)
    if (ActionButton)
    {
        ActionButton->OnClicked.AddDynamic(this, &UUI_ShopSlot::OnActionButtonClicked);
    }
}

void UUI_ShopSlot::BuildNativeTree()
{
    using namespace QuestUI;

    USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Root"));
    Root->SetHeightOverride(Height);
    WidgetTree->RootWidget = Root;

    // 줄 전체가 버튼: 마우스를 올리면 테두리가 금색으로 밝아진다
    ActionButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RowButton"));
    ActionButton->SetStyle(MakeButtonStyle(SRGB(0x15, 0x11, 0x0C), SRGB(0x26, 0x1E, 0x13), SRGB(0x10, 0x0D, 0x09), SRGB(0x5E, 0x4B, 0x2E), Gold));
    Root->SetContent(ActionButton);

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));

    if (UPanelSlot* ButtonContent = ActionButton->SetContent(Row))
    {
        if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(ButtonContent))
        {
            ContentSlot->SetHorizontalAlignment(HAlign_Fill);
            ContentSlot->SetVerticalAlignment(VAlign_Fill);
            ContentSlot->SetPadding(FMargin(8.f, 0.f, 10.f, 0.f));
        }
    }

    // ---- 아이콘 칸 ----
    if (UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(PanelUI::MakeIconBox(WidgetTree, TEXT("Icon"), 46.f, 10, IconBox)))
    {
        IconSlot->SetVerticalAlignment(VAlign_Center);
        IconSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
    }

    ItemIcon = IconBox.Icon.Get();

    // ---- 이름 / 설명 ----
    UVerticalBox* Texts = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Texts"));

    NameText = MakeText(WidgetTree, TEXT("NativeNameText"), 16, Ivory, true);
    Texts->AddChildToVerticalBox(NameText);

    SubText = MakeText(WidgetTree, TEXT("SubText"), 12, Muted, false);

    if (UVerticalBoxSlot* SubSlot = Texts->AddChildToVerticalBox(SubText))
        SubSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));

    if (UHorizontalBoxSlot* TextsSlot = Row->AddChildToHorizontalBox(Texts))
    {
        TextsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        TextsSlot->SetVerticalAlignment(VAlign_Center);
    }

    // ---- 값 / 보유 수 ----
    UVerticalBox* Prices = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Prices"));

    UTextBlock* PriceLine = nullptr;

    if (UHorizontalBox* Badge = PanelUI::MakeGoldBadge(WidgetTree, TEXT("PriceBadge"), 16, PriceLine))
    {
        PriceText = PriceLine;

        if (UVerticalBoxSlot* BadgeSlot = Prices->AddChildToVerticalBox(Badge))
            BadgeSlot->SetHorizontalAlignment(HAlign_Right);
    }

    CountText = MakeText(WidgetTree, TEXT("NativeCountText"), 12, Muted, false);
    CountText->SetJustification(ETextJustify::Right);

    if (UVerticalBoxSlot* CountSlot = Prices->AddChildToVerticalBox(CountText))
    {
        CountSlot->SetHorizontalAlignment(HAlign_Right);
        CountSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
    }

    if (UHorizontalBoxSlot* PricesSlot = Row->AddChildToHorizontalBox(Prices))
    {
        PricesSlot->SetVerticalAlignment(VAlign_Center);
        PricesSlot->SetPadding(FMargin(12.f, 0.f, 12.f, 0.f));
    }

    // ---- 구매/판매 표시 ----
    USizeBox* PillBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PillBox"));
    PillBox->SetWidthOverride(88.f);
    PillBox->SetHeightOverride(34.f);

    ActionPill = MakeBorder(WidgetTree, TEXT("ActionPill"), MakeBoxBrush(SRGB(0x3E, 0x2F, 0x14), Gold, 1.5f, 5.f));
    ActionPill->SetHorizontalAlignment(HAlign_Center);
    ActionPill->SetVerticalAlignment(VAlign_Center);
    PillBox->SetContent(ActionPill);

    ActionLabel = MakeText(WidgetTree, TEXT("ActionLabel"), 14, GoldBright, true, true, 1);
    ActionLabel->SetJustification(ETextJustify::Center);
    ActionPill->SetContent(ActionLabel);

    if (UHorizontalBoxSlot* PillSlot = Row->AddChildToHorizontalBox(PillBox))
        PillSlot->SetVerticalAlignment(VAlign_Center);

    bBuiltNative = true;
}

void UUI_ShopSlot::InitSlot(FName InItemID, const FItemData* ItemData, EShopSlotMode InMode, const FShopRowInfo& Info)
{
    CurrentItemID = InItemID;
    CurrentMode = InMode;

    const bool bBlocked = Info.Block != EShopBlock::None;
    const FText Name = ItemData && !ItemData->DisplayName.IsEmpty() ? ItemData->DisplayName : FText::FromName(CurrentItemID);

    if (bBuiltNative)
    {
        FEquipInfo Equip;
        const bool bEquipment = PanelUI::ResolveEquip(CurrentItemID, ItemData, Equip);

        IconBox.Set(PanelUI::LoadItemIcon(CurrentItemID, ItemData), Name, PanelUI::ItemAccent(ItemData, bEquipment));
    }
    else if (ItemIcon && ItemData && !ItemData->Icon.IsNull())
    {
        if (UTexture2D* IconTexture = ItemData->Icon.LoadSynchronous())
        {
            ItemIcon->SetBrushFromTexture(IconTexture);
        }
    }

    if (NameText)
    {
        NameText->SetText(Name);
    }

    if (SubText)
    {
        SubText->SetText(FText::FromString(PanelUI::ItemSummary(CurrentItemID, ItemData)));
    }

    if (PriceText)
    {
        PriceText->SetText(FText::FromString(bBuiltNative ? PanelUI::Num(Info.TotalPrice) : FString::FromInt(Info.TotalPrice)));

        // 골드가 모자라 못 사는 줄은 값을 붉게 보여준다
        if (bBuiltNative)
        {
            PriceText->SetColorAndOpacity(FSlateColor(bBlocked && InMode == EShopSlotMode::Buy ? PanelUI::Danger : QuestUI::GoldBright));
        }
    }

    if (CountText)
    {
        if (bBuiltNative)
        {
            CountText->SetText(FText::FromString(FString::Printf(TEXT("보유 %d"), Info.OwnedCount)));
        }
        else if (CurrentMode == EShopSlotMode::Sell)
        {
            CountText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
            CountText->SetText(FText::AsNumber(Info.OwnedCount));
        }
        else
        {
            CountText->SetVisibility(ESlateVisibility::Hidden);
        }
    }

    if (ActionPill && ActionLabel)
    {
        using namespace QuestUI;

        FString Label = InMode == EShopSlotMode::Buy ? TEXT("구매") : TEXT("판매");

        if (Info.Quantity > 1)
            Label += FString::Printf(TEXT(" ×%d"), Info.Quantity);

        switch (Info.Block)
        {
        case EShopBlock::NoGold:	Label = TEXT("골드 부족"); break;
        case EShopBlock::NoRoom:	Label = TEXT("가방 가득"); break;
        case EShopBlock::NoStock:	Label = TEXT("수량 없음"); break;
        default: break;
        }

        ActionLabel->SetText(FText::FromString(Label));

        if (bBlocked)
        {
            ActionPill->SetBrush(MakeBoxBrush(SRGB(0x1A, 0x14, 0x10), SRGB(0x5E, 0x4B, 0x2E), 1.5f, 5.f));
            ActionLabel->SetColorAndOpacity(FSlateColor(PanelUI::Danger));
        }
        else
        {
            ActionPill->SetBrush(MakeBoxBrush(SRGB(0x3E, 0x2F, 0x14), Gold, 1.5f, 5.f));
            ActionLabel->SetColorAndOpacity(FSlateColor(GoldBright));
        }
    }
}

void UUI_ShopSlot::OnActionButtonClicked()
{
    OnTradeRequested.Broadcast(this);
}
