#include "UI_Shop.h"
#include "UI_ShopSlot.h"
#include "UI_PanelParts.h"
#include "InventoryComponent.h"
#include "CapstoneSFX.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

namespace
{
	// 익명 네임스페이스라도 unity 빌드에서 UI_Enhance.cpp와 한 덩어리로 묶이면 이름이 충돌하므로 Shop 접두어를 붙인다
	constexpr float ShopPanelWidth = 560.f;
	constexpr int32 VisibleRows = 4;
	constexpr float RowGap = 6.f;
	constexpr float ShopToastSeconds = 3.f;
	constexpr float ShopToastFadeSeconds = 0.8f;
}

void UUI_Shop::InitShop(const TArray<FName>& InShopItemIDs, UInventoryComponent* InInventoryComp)
{
    ShopItemIDs = InShopItemIDs;

    if (InventoryComp)
    {
        InventoryComp->OnInventoryChanged.RemoveDynamic(this, &UUI_Shop::HandleInventoryChanged);
    }

    InventoryComp = InInventoryComp;

    if (InventoryComp)
    {
        InventoryComp->OnInventoryChanged.AddUniqueDynamic(this, &UUI_Shop::HandleInventoryChanged);
    }

    ShowHint();
    RefreshShopUI();
}

void UUI_Shop::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // Esc로 닫으려면 이 위젯이 키보드 포커스를 받을 수 있어야 한다
    SetIsFocusable(true);

    // 위젯 블루프린트의 트리를 통째로 코드 조립 트리로 교체한다. 디자이너 미리보기가 아니라 실제 인스턴스에서만
    if (bUseCodeBuiltLayout && WidgetTree && !IsDesignTime())
    {
        BuildNativeTree();

        // 화면 한가운데에 놓는다. SetPositionInViewport는 앵커를 (0,0)으로 되돌리므로 쓰지 않는다
        SetAnchorsInViewport(FAnchors(0.5f, 0.5f));
        SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
    }

    // 버튼 연결은 한 번만 한다 (창을 열 때마다 불리는 NativeConstruct에서 하면 중복으로 연결된다)
    if (CloseButton)
    {
        CloseButton->OnClicked.AddDynamic(this, &UUI_Shop::OnCloseButtonClicked);
    }

    if (BuyTabButton)
    {
        BuyTabButton->OnClicked.AddDynamic(this, &UUI_Shop::HandleBuyTab);
    }

    if (SellTabButton)
    {
        SellTabButton->OnClicked.AddDynamic(this, &UUI_Shop::HandleSellTab);
    }

    if (QuantityButtons.Num() == 3)
    {
        QuantityButtons[0]->OnClicked.AddDynamic(this, &UUI_Shop::HandleQuantityOne);
        QuantityButtons[1]->OnClicked.AddDynamic(this, &UUI_Shop::HandleQuantityTen);
        QuantityButtons[2]->OnClicked.AddDynamic(this, &UUI_Shop::HandleQuantityMax);
    }
}

void UUI_Shop::NativeConstruct()
{
    Super::NativeConstruct();

    FocusTriesLeft = 8;
}

void UUI_Shop::NativeDestruct()
{
    if (InventoryComp)
    {
        InventoryComp->OnInventoryChanged.RemoveDynamic(this, &UUI_Shop::HandleInventoryChanged);
    }

    Super::NativeDestruct();
}

void UUI_Shop::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (FocusTriesLeft > 0)
    {
        --FocusTriesLeft;

        if (!HasFocusedDescendants() && !HasKeyboardFocus())
            SetKeyboardFocus();
        else
            FocusTriesLeft = 0;
    }

    if (bRefreshPending)
    {
        bRefreshPending = false;
        RefreshShopUI();
    }

    if (ToastRemaining > 0.f && ToastText)
    {
        ToastRemaining -= InDeltaTime;

        if (ToastRemaining <= 0.f)
            ShowHint();
        else if (ToastRemaining < ShopToastFadeSeconds)
            ToastText->SetOpacity(ToastRemaining / ShopToastFadeSeconds);
    }
}

FReply UUI_Shop::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        OnShopClosed.Broadcast();
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UUI_Shop::BuildNativeTree()
{
    using namespace QuestUI;

    const PanelUI::FPanelFrame Frame = PanelUI::BuildFrame(WidgetTree, ShopPanelWidth, TEXT("상    점"), TEXT("[Esc] 닫기"));
    CloseButton = Frame.CloseButton;
    UVerticalBox* Column = Frame.Column;

    // ---- 탭(구매/판매) + 수량 칩 ----
    UHorizontalBox* Bar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Bar"));

    UButton* Button = nullptr;
    UTextBlock* Label = nullptr;

    if (UHorizontalBoxSlot* BuySlot = Bar->AddChildToHorizontalBox(PanelUI::MakeTextButton(WidgetTree, TEXT("BuyTab"), TEXT("구  매"), 16, 104.f, 36.f, Button, Label)))
        BuySlot->SetVerticalAlignment(VAlign_Center);

    BuyTabButton = Button;
    BuyTabLabel = Label;

    if (UHorizontalBoxSlot* SellSlot = Bar->AddChildToHorizontalBox(PanelUI::MakeTextButton(WidgetTree, TEXT("SellTab"), TEXT("판  매"), 16, 104.f, 36.f, Button, Label)))
    {
        SellSlot->SetVerticalAlignment(VAlign_Center);
        SellSlot->SetPadding(FMargin(6.f, 0.f, 0.f, 0.f));
    }

    SellTabButton = Button;
    SellTabLabel = Label;

    // 탭과 수량 칩 사이를 벌리는 빈 칸
    if (UHorizontalBoxSlot* SpacerSlot = Bar->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BarSpacer"))))
        SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UTextBlock* QuantityTitle = MakeText(WidgetTree, TEXT("QuantityTitle"), 13, Muted, false, true, 1);
    QuantityTitle->SetText(FText::FromString(TEXT("수량")));

    if (UHorizontalBoxSlot* TitleSlot = Bar->AddChildToHorizontalBox(QuantityTitle))
    {
        TitleSlot->SetVerticalAlignment(VAlign_Center);
        TitleSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
    }

    const TCHAR* ChipNames[3] = { TEXT("QtyOne"), TEXT("QtyTen"), TEXT("QtyMax") };
    const TCHAR* ChipLabels[3] = { TEXT("×1"), TEXT("×10"), TEXT("최대") };

    for (int32 i = 0; i < 3; ++i)
    {
        if (UHorizontalBoxSlot* ChipSlot = Bar->AddChildToHorizontalBox(PanelUI::MakeTextButton(WidgetTree, ChipNames[i], ChipLabels[i], 14, 54.f, 30.f, Button, Label)))
        {
            ChipSlot->SetVerticalAlignment(VAlign_Center);
            ChipSlot->SetPadding(FMargin(i == 0 ? 0.f : 4.f, 0.f, 0.f, 0.f));
        }

        QuantityButtons.Add(Button);
        QuantityLabels.Add(Label);
    }

    if (UVerticalBoxSlot* BarSlot = Column->AddChildToVerticalBox(Bar))
        BarSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

    // ---- 목록 ----
    UBorder* ListFrame = MakeBorder(WidgetTree, TEXT("ListFrame"), MakeBoxBrush(InsetFill, GoldDim, 1.5f, 6.f), FMargin(8.f, 8.f, 8.f, 2.f));
    Column->AddChildToVerticalBox(ListFrame);

    // 줄 수가 달라도 창 높이가 흔들리지 않게 보이는 높이를 고정하고, 넘치면 스크롤한다
    USizeBox* ListBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ListBox"));
    ListBox->SetHeightOverride(VisibleRows * (UUI_ShopSlot::Height + RowGap));
    ListFrame->SetContent(ListBox);

    ListScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ListScroll"));
    ListBox->SetContent(ListScroll);

    ItemList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ItemList"));
    ListScroll->AddChild(ItemList);

    // ---- 메시지 한 줄 ----
    ToastText = MakeText(WidgetTree, TEXT("ToastText"), 13, Muted, false, true, 1);

    USizeBox* ToastBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ToastBox"));
    ToastBox->SetHeightOverride(22.f);
    ToastBox->SetContent(ToastText);

    if (UVerticalBoxSlot* ToastSlot = Column->AddChildToVerticalBox(ToastBox))
        ToastSlot->SetPadding(FMargin(4.f, 10.f, 0.f, 0.f));

    // ---- 아래줄: 소지품 수 / 소지금 ----
    if (UVerticalBoxSlot* FootLineSlot = Column->AddChildToVerticalBox(MakeLine(WidgetTree, TEXT("FootLine"), GoldDim, 1.f, true)))
        FootLineSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 10.f));

    UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Footer"));

    CapacityText = MakeText(WidgetTree, TEXT("CapacityText"), 13, Muted, false, true, 1);

    if (UHorizontalBoxSlot* CapacitySlot = Footer->AddChildToHorizontalBox(CapacityText))
    {
        CapacitySlot->SetVerticalAlignment(VAlign_Center);
        CapacitySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    UTextBlock* GoldValue = nullptr;
    Footer->AddChildToHorizontalBox(PanelUI::MakeGoldBadge(WidgetTree, TEXT("GoldBadge"), 18, GoldValue));
    GoldText = GoldValue;

    Column->AddChildToVerticalBox(Footer);
}

void UUI_Shop::RefreshShopUI()
{
    if (!InventoryComp)
        return;

    if (ItemList)
    {
        // 목록을 다시 그려도 보던 자리에서 스크롤이 튀지 않게 위치를 기억해 둔다
        const float Offset = ListScroll ? ListScroll->GetScrollOffset() : 0.f;

        PopulatePanel(ItemList, Mode);

        if (ListScroll)
            ListScroll->SetScrollOffset(Offset);

        PanelUI::ApplyToggleStyle(BuyTabButton, BuyTabLabel, Mode == EShopSlotMode::Buy);
        PanelUI::ApplyToggleStyle(SellTabButton, SellTabLabel, Mode == EShopSlotMode::Sell);

        for (int32 i = 0; i < QuantityButtons.Num(); ++i)
            PanelUI::ApplyToggleStyle(QuantityButtons[i], QuantityLabels[i], static_cast<int32>(Quantity) == i);

        if (CapacityText)
            CapacityText->SetText(FText::FromString(FString::Printf(TEXT("소지품   %d / %d"), InventoryComp->Items.Num(), UInventoryComponent::MaxSlots)));
    }
    else
    {
        PopulatePanel(BuyItemsPanel, EShopSlotMode::Buy);
        PopulatePanel(SellItemsPanel, EShopSlotMode::Sell);
    }

    if (GoldText)
    {
        // 코드 조립 레이아웃은 금화 그림 옆에 숫자만 쓴다
        GoldText->SetText(FText::FromString(ItemList ? FString::Printf(TEXT("%s G"), *PanelUI::Num(InventoryComp->GetGold())) : FString::FromInt(InventoryComp->GetGold())));
    }
}

void UUI_Shop::PopulatePanel(UPanelWidget* Panel, EShopSlotMode SlotMode)
{
    if (!Panel || !InventoryComp)
        return;

    Panel->ClearChildren();

    const TSubclassOf<UUI_ShopSlot> SlotClass = ShopSlotClass ? ShopSlotClass : TSubclassOf<UUI_ShopSlot>(UUI_ShopSlot::StaticClass());

    TArray<FName> ItemIDs;

    if (SlotMode == EShopSlotMode::Buy)
    {
        ItemIDs = ShopItemIDs;
    }
    else
    {
        // 같은 아이템이 여러 스택에 나뉘어 있어도 한 줄로 합쳐 보여준다
        for (const FInventorySlot& InvSlot : InventoryComp->Items)
            ItemIDs.AddUnique(InvSlot.ItemID);
    }

    int32 Shown = 0;

    for (const FName& ItemID : ItemIDs)
    {
        const FItemData* ItemData = InventoryComp->GetItemData(ItemID);

        if (!ItemData || (SlotMode == EShopSlotMode::Sell && ItemData->SellPrice <= 0))
            continue;

        UUI_ShopSlot* RowSlot = CreateWidget<UUI_ShopSlot>(this, SlotClass);

        if (!RowSlot)
            continue;

        RowSlot->InitSlot(ItemID, ItemData, SlotMode, MakeRowInfo(ItemID, *ItemData, SlotMode));
        RowSlot->OnTradeRequested.AddUObject(this, &UUI_Shop::HandleTradeRequested);

        if (UVerticalBoxSlot* BoxSlot = Cast<UVerticalBoxSlot>(Panel->AddChild(RowSlot)))
        {
            BoxSlot->SetHorizontalAlignment(HAlign_Fill);
            BoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RowGap));
        }

        ++Shown;
    }

    // 코드 조립 목록이 비어 있으면 왜 비었는지 알려준다
    if (Shown == 0 && Panel == ItemList)
    {
        // 목록을 다시 그릴 때마다 만들기 때문에 이름을 붙이지 않는다 (같은 이름의 위젯을 두 번 만들 수 없다)
        UTextBlock* EmptyText = QuestUI::MakeText(WidgetTree, TEXT(""), 14, QuestUI::Muted, false);
        EmptyText->SetJustification(ETextJustify::Center);
        EmptyText->SetAutoWrapText(true);
        EmptyText->SetText(FText::FromString(SlotMode == EShopSlotMode::Buy
            ? TEXT("지금은 팔고 있는 물건이 없습니다.")
            : TEXT("팔 수 있는 아이템이 없습니다.\n사냥으로 전리품을 모아 오세요.")));

        if (UVerticalBoxSlot* EmptySlot = Cast<UVerticalBoxSlot>(Panel->AddChild(EmptyText)))
        {
            EmptySlot->SetHorizontalAlignment(HAlign_Fill);
            EmptySlot->SetPadding(FMargin(0.f, 70.f, 0.f, 0.f));
        }
    }
}

FShopRowInfo UUI_Shop::MakeRowInfo(FName ItemID, const FItemData& Data, EShopSlotMode SlotMode) const
{
    FShopRowInfo Info;
    Info.OwnedCount = InventoryComp->GetItemCount(ItemID);

    const bool bWantMax = Quantity == EShopQuantity::Max;
    const int32 Wanted = Quantity == EShopQuantity::Ten ? 10 : 1;

    if (SlotMode == EShopSlotMode::Buy)
    {
        const int32 UnitPrice = FMath::Max(0, Data.BuyPrice);
        const int32 ByGold = UnitPrice > 0 ? InventoryComp->GetGold() / UnitPrice : MaxPerTrade;
        const int32 ByRoom = InventoryComp->GetAddableCount(ItemID);
        const int32 Feasible = FMath::Min3(ByGold, ByRoom, MaxPerTrade);

        if (Feasible <= 0)
        {
            // 못 사는 줄도 값은 보여준다 (골드가 얼마나 모자란지 가늠할 수 있게 원하는 개수 기준)
            Info.Quantity = bWantMax ? 1 : Wanted;
            Info.Block = ByGold <= 0 ? EShopBlock::NoGold : EShopBlock::NoRoom;
        }
        else
        {
            // 골드가 모자라도 살 수 있는 만큼은 산다 (x10을 눌렀는데 4개 값밖에 없으면 4개)
            Info.Quantity = bWantMax ? Feasible : FMath::Min(Wanted, Feasible);
        }

        Info.TotalPrice = UnitPrice * Info.Quantity;
    }
    else
    {
        Info.Quantity = bWantMax ? FMath::Max(1, Info.OwnedCount) : FMath::Min(Wanted, FMath::Max(1, Info.OwnedCount));
        Info.TotalPrice = FMath::Max(0, Data.SellPrice) * Info.Quantity;

        if (Info.OwnedCount <= 0)
            Info.Block = EShopBlock::NoStock;
    }

    return Info;
}

void UUI_Shop::HandleTradeRequested(UUI_ShopSlot* RowSlot)
{
    if (!InventoryComp || !RowSlot)
        return;

    const FName ItemID = RowSlot->GetItemID();
    const EShopSlotMode SlotMode = RowSlot->GetMode();
    const FItemData* ItemData = InventoryComp->GetItemData(ItemID);

    if (!ItemData)
        return;

    // 줄을 그린 뒤 골드/인벤토리가 바뀌었을 수 있으니 누른 순간의 값으로 다시 계산한다
    const FShopRowInfo Info = MakeRowInfo(ItemID, *ItemData, SlotMode);
    const FString Name = (ItemData->DisplayName.IsEmpty() ? FText::FromName(ItemID) : ItemData->DisplayName).ToString();

    if (Info.Block != EShopBlock::None)
    {
        CapstoneSFX::Play2D(this, ESFX::UI_Deny);

        switch (Info.Block)
        {
        case EShopBlock::NoGold:	ShowToast(TEXT("골드가 부족합니다."), false); break;
        case EShopBlock::NoRoom:	ShowToast(TEXT("소지품이 가득 차서 더 살 수 없습니다."), false); break;
        default:					ShowToast(TEXT("팔 수 있는 물건이 없습니다."), false); break;
        }

        return;
    }

    if (SlotMode == EShopSlotMode::Buy)
    {
        if (!InventoryComp->TrySpendGold(Info.TotalPrice))
        {
            CapstoneSFX::Play2D(this, ESFX::UI_Deny);
            ShowToast(TEXT("골드가 부족합니다."), false);
            return;
        }

        InventoryComp->AddItem(ItemID, Info.Quantity);
        CapstoneSFX::Play2D(this, ESFX::UI_Coin);
        ShowToast(FString::Printf(TEXT("%s ×%d 구매   -%s G"), *Name, Info.Quantity, *PanelUI::Num(Info.TotalPrice)), true);
    }
    else
    {
        if (!InventoryComp->RemoveItem(ItemID, Info.Quantity))
            return;

        InventoryComp->AddGold(Info.TotalPrice);
        CapstoneSFX::Play2D(this, ESFX::UI_Sell);
        ShowToast(FString::Printf(TEXT("%s ×%d 판매   +%s G"), *Name, Info.Quantity, *PanelUI::Num(Info.TotalPrice)), true);
    }
}

void UUI_Shop::SetMode(EShopSlotMode NewMode)
{
    if (Mode == NewMode)
        return;

    Mode = NewMode;

    ShowHint();
    RefreshShopUI();

    // 다른 탭의 목록은 처음부터 보여준다
    if (ListScroll)
        ListScroll->ScrollToStart();
}

void UUI_Shop::SetQuantity(EShopQuantity NewQuantity)
{
    Quantity = NewQuantity;
    RefreshShopUI();
}

void UUI_Shop::ShowToast(const FString& Message, bool bGood)
{
    if (!ToastText)
        return;

    ToastText->SetText(FText::FromString(Message));
    ToastText->SetColorAndOpacity(FSlateColor(bGood ? QuestUI::Success : PanelUI::Danger));
    ToastText->SetOpacity(1.f);

    ToastRemaining = ShopToastSeconds;
}

void UUI_Shop::ShowHint()
{
    if (!ToastText)
        return;

    ToastText->SetText(FText::FromString(Mode == EShopSlotMode::Buy
        ? TEXT("사고 싶은 물건을 누르면 선택한 수량만큼 바로 삽니다.")
        : TEXT("팔고 싶은 물건을 누르면 선택한 수량만큼 바로 팝니다.")));
    ToastText->SetColorAndOpacity(FSlateColor(QuestUI::Muted));
    ToastText->SetOpacity(1.f);

    ToastRemaining = 0.f;
}

void UUI_Shop::HandleInventoryChanged()
{
    bRefreshPending = true;
}

void UUI_Shop::OnCloseButtonClicked()
{
    OnShopClosed.Broadcast();
}

void UUI_Shop::HandleBuyTab()
{
    SetMode(EShopSlotMode::Buy);
}

void UUI_Shop::HandleSellTab()
{
    SetMode(EShopSlotMode::Sell);
}

void UUI_Shop::HandleQuantityOne()
{
    SetQuantity(EShopQuantity::One);
}

void UUI_Shop::HandleQuantityTen()
{
    SetQuantity(EShopQuantity::Ten);
}

void UUI_Shop::HandleQuantityMax()
{
    SetQuantity(EShopQuantity::Max);
}
