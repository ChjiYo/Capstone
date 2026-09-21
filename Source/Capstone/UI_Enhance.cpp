#include "UI_Enhance.h"
#include "Py_Grey.h"
#include "InventoryComponent.h"
#include "EquipmentComponent.h"
#include "ItemData.h"
#include "CapstoneSFX.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"

namespace
{
	constexpr float PanelWidth = 440.f;
	constexpr float ToastSeconds = 4.f;
	constexpr float ToastFadeSeconds = 0.8f;
	constexpr float FlashSeconds = 0.8f;
}

void UUI_Enhance::InitEnhance(APy_Grey* InPlayer)
{
    if (Player && Player->InventoryComponent)
    {
        Player->InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UUI_Enhance::HandleInventoryChanged);
    }

    APy_Grey::OnWeaponEnhanced.Remove(WeaponEnhancedHandle);

    Player = InPlayer;

    if (Player && Player->InventoryComponent)
    {
        Player->InventoryComponent->OnInventoryChanged.AddUniqueDynamic(this, &UUI_Enhance::HandleInventoryChanged);
    }

    WeaponEnhancedHandle = APy_Grey::OnWeaponEnhanced.AddUObject(this, &UUI_Enhance::HandleWeaponEnhanced);

    if (bBuiltNative && Player)
    {
        RebuildPips(Player->MaxWeaponLevel);
    }

    ShowHint();
    RefreshEnhanceUI();
}

void UUI_Enhance::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // Esc/Enter를 받으려면 이 위젯이 키보드 포커스를 받을 수 있어야 한다
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
    if (EnhanceButton)
    {
        EnhanceButton->OnClicked.AddDynamic(this, &UUI_Enhance::OnEnhanceButtonClicked);
    }

    if (CloseButton)
    {
        CloseButton->OnClicked.AddDynamic(this, &UUI_Enhance::OnCloseButtonClicked);
    }
}

void UUI_Enhance::NativeConstruct()
{
    Super::NativeConstruct();

    FocusTriesLeft = 8;
}

void UUI_Enhance::NativeDestruct()
{
    if (Player && Player->InventoryComponent)
    {
        Player->InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UUI_Enhance::HandleInventoryChanged);
    }

    APy_Grey::OnWeaponEnhanced.Remove(WeaponEnhancedHandle);

    Super::NativeDestruct();
}

void UUI_Enhance::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
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

    if (FlashRemaining > 0.f)
    {
        FlashRemaining = FMath::Max(0.f, FlashRemaining - InDeltaTime);
        PaintHero(FlashRemaining / FlashSeconds);
    }

    if (ToastRemaining > 0.f && ToastText)
    {
        ToastRemaining -= InDeltaTime;

        if (ToastRemaining <= 0.f)
            ShowHint();
        else if (ToastRemaining < ToastFadeSeconds)
            ToastText->SetOpacity(ToastRemaining / ToastFadeSeconds);
    }
}

FReply UUI_Enhance::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        OnEnhanceClosed.Broadcast();
        return FReply::Handled();
    }

    if (InKeyEvent.GetKey() == EKeys::Enter)
    {
        OnEnhanceButtonClicked();
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UUI_Enhance::BuildNativeTree()
{
    using namespace QuestUI;

    const PanelUI::FPanelFrame Frame = PanelUI::BuildFrame(WidgetTree, PanelWidth, TEXT("무 기   강 화"), TEXT("[Esc] 닫기"));
    CloseButton = Frame.CloseButton;
    UVerticalBox* Column = Frame.Column;

    // ---- 지금 무기: 아이콘 + 이름 + 강화 단계(+N → +N+1) + 단계 마름모 ----
    UHorizontalBox* Hero = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Hero"));

    if (UHorizontalBoxSlot* IconSlot = Hero->AddChildToHorizontalBox(PanelUI::MakeIconBox(WidgetTree, TEXT("HeroIcon"), 92.f, 12, HeroIcon)))
    {
        IconSlot->SetVerticalAlignment(VAlign_Center);
        IconSlot->SetPadding(FMargin(4.f, 0.f, 20.f, 0.f));
    }

    UVerticalBox* HeroTexts = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HeroTexts"));

    WeaponNameText = MakeText(WidgetTree, TEXT("WeaponName"), 16, Ivory, true);
    HeroTexts->AddChildToVerticalBox(WeaponNameText);

    UHorizontalBox* LevelRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("LevelRow"));

    WeaponLevelText = MakeText(WidgetTree, TEXT("LevelText"), 36, GoldBright, true);

    if (UHorizontalBoxSlot* LevelSlot = LevelRow->AddChildToHorizontalBox(WeaponLevelText))
        LevelSlot->SetVerticalAlignment(VAlign_Center);

    NextLevelText = MakeText(WidgetTree, TEXT("NextLevelText"), 20, Success, true);

    if (UHorizontalBoxSlot* NextSlot = LevelRow->AddChildToHorizontalBox(NextLevelText))
    {
        NextSlot->SetVerticalAlignment(VAlign_Center);
        NextSlot->SetPadding(FMargin(14.f, 0.f, 0.f, 0.f));
    }

    HeroTexts->AddChildToVerticalBox(LevelRow);

    PipRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PipRow"));

    if (UVerticalBoxSlot* PipSlot = HeroTexts->AddChildToVerticalBox(PipRow))
        PipSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));

    if (UHorizontalBoxSlot* HeroTextsSlot = Hero->AddChildToHorizontalBox(HeroTexts))
    {
        HeroTextsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        HeroTextsSlot->SetVerticalAlignment(VAlign_Center);
    }

    Column->AddChildToVerticalBox(Hero);

    if (UVerticalBoxSlot* LineSlot = Column->AddChildToVerticalBox(MakeLine(WidgetTree, TEXT("AttackLine"), GoldDim, 1.f, true)))
        LineSlot->SetPadding(FMargin(0.f, 18.f, 0.f, 14.f));

    // ---- 공격력: 지금 → 강화 후 ----
    UHorizontalBox* AttackRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AttackRow"));

    UTextBlock* AttackLabel = MakeText(WidgetTree, TEXT("AttackLabel"), 16, Gold, true, true, 2);
    AttackLabel->SetText(FText::FromString(TEXT("공격력")));

    if (UHorizontalBoxSlot* LabelSlot = AttackRow->AddChildToHorizontalBox(AttackLabel))
    {
        LabelSlot->SetVerticalAlignment(VAlign_Center);
        LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    AttackPowerText = MakeText(WidgetTree, TEXT("AttackNow"), 22, Ivory, true);

    if (UHorizontalBoxSlot* NowSlot = AttackRow->AddChildToHorizontalBox(AttackPowerText))
        NowSlot->SetVerticalAlignment(VAlign_Center);

    AttackNextText = MakeText(WidgetTree, TEXT("AttackNext"), 16, Success, true);

    if (UHorizontalBoxSlot* NextAttackSlot = AttackRow->AddChildToHorizontalBox(AttackNextText))
    {
        NextAttackSlot->SetVerticalAlignment(VAlign_Center);
        NextAttackSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
    }

    Column->AddChildToVerticalBox(AttackRow);

    if (UVerticalBoxSlot* LineSlot2 = Column->AddChildToVerticalBox(MakeLine(WidgetTree, TEXT("CostLine"), GoldDim, 1.f, true)))
        LineSlot2->SetPadding(FMargin(0.f, 14.f, 0.f, 12.f));

    // ---- 필요한 것: 재료 / 골드 (보유 / 필요) ----
    UTextBlock* CostTitle = MakeText(WidgetTree, TEXT("CostTitle"), 13, Muted, false, true, 1);
    CostTitle->SetText(FText::FromString(TEXT("필요한 것   (보유 / 필요)")));

    if (UVerticalBoxSlot* CostTitleSlot = Column->AddChildToVerticalBox(CostTitle))
        CostTitleSlot->SetPadding(FMargin(2.f, 0.f, 0.f, 8.f));

    auto AddRequirement = [&](const TCHAR* Name, PanelUI::FIconBox& Box, TObjectPtr<UTextBlock>& OutName, UTextBlock*& OutValue)
    {
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), FName(*FString::Printf(TEXT("%sRow"), Name)));

        if (UHorizontalBoxSlot* BoxSlot = Row->AddChildToHorizontalBox(PanelUI::MakeIconBox(WidgetTree, *FString::Printf(TEXT("%sIcon"), Name), 44.f, 14, Box)))
        {
            BoxSlot->SetVerticalAlignment(VAlign_Center);
            BoxSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
        }

        OutName = MakeText(WidgetTree, *FString::Printf(TEXT("%sName"), Name), 16, Ivory, false);

        if (UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(OutName))
        {
            NameSlot->SetVerticalAlignment(VAlign_Center);
            NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }

        OutValue = MakeText(WidgetTree, *FString::Printf(TEXT("%sValue"), Name), 18, Ivory, true);

        if (UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(OutValue))
            ValueSlot->SetVerticalAlignment(VAlign_Center);

        if (UVerticalBoxSlot* RowSlot = Column->AddChildToVerticalBox(Row))
            RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
    };

    UTextBlock* MaterialValue = nullptr;
    AddRequirement(TEXT("Material"), MaterialBox, MaterialNameText, MaterialValue);
    MaterialCostText = MaterialValue;
    MaterialIcon = MaterialBox.Icon.Get();

    TObjectPtr<UTextBlock> GoldNameText;
    UTextBlock* GoldValue = nullptr;
    AddRequirement(TEXT("Gold"), GoldBox, GoldNameText, GoldValue);
    GoldCostText = GoldValue;
    GoldNameText->SetText(FText::FromString(TEXT("골드")));
    GoldBox.Set(nullptr, FText::FromString(TEXT("G")), Gold);

    // ---- 강화하기 버튼 ----
    UButton* Button = nullptr;
    UTextBlock* Label = nullptr;
    USizeBox* ButtonBox = PanelUI::MakeTextButton(WidgetTree, TEXT("NativeEnhanceButton"), TEXT("강  화  하  기"), 20, 0.f, 54.f, Button, Label);
    PanelUI::ApplyPrimaryStyle(Button, Label);
    EnhanceButton = Button;
    EnhanceLabel = Label;

    if (UVerticalBoxSlot* ButtonSlot = Column->AddChildToVerticalBox(ButtonBox))
        ButtonSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));

    // ---- 메시지 한 줄 ----
    ToastText = MakeText(WidgetTree, TEXT("ToastText"), 13, Muted, false, true, 1);
    ToastText->SetJustification(ETextJustify::Center);

    USizeBox* ToastBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ToastBox"));
    ToastBox->SetHeightOverride(22.f);
    ToastBox->SetContent(ToastText);

    if (UVerticalBoxSlot* ToastSlot = Column->AddChildToVerticalBox(ToastBox))
    {
        ToastSlot->SetHorizontalAlignment(HAlign_Center);
        ToastSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
    }

    bBuiltNative = true;
}

void UUI_Enhance::RebuildPips(int32 MaxLevel)
{
    if (!PipRow)
        return;

    using namespace QuestUI;

    PipRow->ClearChildren();
    PipShapes.Reset();

    for (int32 i = 0; i < MaxLevel; ++i)
    {
        // 다시 만들 수 있으니 이름을 붙이지 않는다 (같은 이름의 위젯을 두 번 만들 수 없다)
        USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        Box->SetWidthOverride(11.f);
        Box->SetHeightOverride(11.f);

        UBorder* Shape = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        Shape->SetRenderTransformAngle(45.f);
        Box->SetContent(Shape);

        PipShapes.Add(Shape);

        if (UHorizontalBoxSlot* PipSlot = PipRow->AddChildToHorizontalBox(Box))
            PipSlot->SetPadding(FMargin(i == 0 ? 4.f : 12.f, 4.f, 0.f, 4.f));
    }
}

void UUI_Enhance::PaintHero(float Glow)
{
    HeroIcon.Paint(QuestUI::Gold, Glow);

    if (WeaponLevelText)
        WeaponLevelText->SetColorAndOpacity(FSlateColor(PanelUI::Blend(QuestUI::GoldBright, FLinearColor::White, Glow)));
}

void UUI_Enhance::RefreshEnhanceUI()
{
    if (!Player || !Player->InventoryComponent)
        return;

    using namespace QuestUI;

    UInventoryComponent* Inventory = Player->InventoryComponent;

    const int32 Level = Player->WeaponLevel;
    const int32 MaxLevel = Player->MaxWeaponLevel;
    const bool bMax = Level >= MaxLevel;
    const int32 Attack = Player->GetAttackPower();

    if (WeaponLevelText)
    {
        WeaponLevelText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), Level)));
    }

    if (AttackPowerText)
    {
        AttackPowerText->SetText(FText::AsNumber(Attack));
    }

    int32 OwnedMaterial = Inventory->GetItemCount(Player->EnhanceMaterialID);
    int32 GoldCost = Player->GetNextEnhanceGoldCost();
    const int32 OwnedGold = Inventory->GetGold();

    const bool bHasMaterial = OwnedMaterial >= Player->EnhanceMaterialCount;
    const bool bHasGold = OwnedGold >= GoldCost;

    const FItemData* MaterialData = Inventory->GetItemData(Player->EnhanceMaterialID);

    if (bBuiltNative)
    {
        MaterialBox.Set(PanelUI::LoadItemIcon(Player->EnhanceMaterialID, MaterialData),
            MaterialData && !MaterialData->DisplayName.IsEmpty() ? MaterialData->DisplayName : FText::FromName(Player->EnhanceMaterialID),
            PanelUI::ItemAccent(MaterialData, false));
    }
    else if (MaterialIcon && MaterialData && !MaterialData->Icon.IsNull())
    {
        if (UTexture2D* IconTexture = MaterialData->Icon.LoadSynchronous())
        {
            MaterialIcon->SetBrushFromTexture(IconTexture);
        }
    }

    if (MaterialCostText)
    {
        MaterialCostText->SetText(FText::FromString(bMax ? FString(TEXT("-")) : FString::Printf(TEXT("%d / %d"), OwnedMaterial, Player->EnhanceMaterialCount)));

        if (bBuiltNative)
            MaterialCostText->SetColorAndOpacity(FSlateColor(bMax ? Muted : (bHasMaterial ? Success : PanelUI::Danger)));
    }

    if (GoldCostText)
    {
        GoldCostText->SetText(FText::FromString(bMax ? FString(TEXT("-")) : FString::Printf(TEXT("%s / %s"), *PanelUI::Num(OwnedGold), *PanelUI::Num(GoldCost))));

        if (bBuiltNative)
            GoldCostText->SetColorAndOpacity(FSlateColor(bMax ? Muted : (bHasGold ? Success : PanelUI::Danger)));
    }

    if (EnhanceButton)
    {
        EnhanceButton->SetIsEnabled(Player->CanEnhanceWeapon());
    }

    if (!bBuiltNative)
        return;

    // ---- 코드 조립 레이아웃에만 있는 요소 ----
    UEquipmentComponent* Equipment = Player->EquipmentComponent;

    FName WeaponID = Equipment ? Equipment->GetEquippedItem(EEquipSlot::Weapon) : NAME_None;

    if (WeaponID.IsNone())
        WeaponID = EquipDefs::GreystoneSword;

    FEquipInfo WeaponInfo;
    const FText WeaponName = Equipment && Equipment->GetEquipInfo(WeaponID, WeaponInfo) ? WeaponInfo.Name : FText::FromString(TEXT("무기"));

    HeroIcon.Set(Equipment ? Equipment->ResolveIcon(WeaponID) : nullptr, WeaponName, Gold);
    PaintHero(FlashRemaining / FlashSeconds);

    if (WeaponNameText)
        WeaponNameText->SetText(WeaponName);

    if (NextLevelText)
    {
        NextLevelText->SetText(FText::FromString(bMax ? FString(TEXT("MAX")) : FString::Printf(TEXT("→  +%d"), Level + 1)));
        NextLevelText->SetColorAndOpacity(FSlateColor(bMax ? GoldBright : Success));
    }

    if (AttackNextText)
    {
        AttackNextText->SetText(FText::FromString(bMax ? FString(TEXT("최대 강화")) : FString::Printf(TEXT("→  %d   (+%d)"), Attack + Player->AttackPerEnhance, Player->AttackPerEnhance)));
        AttackNextText->SetColorAndOpacity(FSlateColor(bMax ? GoldBright : Success));
    }

    if (MaterialNameText)
    {
        MaterialNameText->SetText(MaterialData && !MaterialData->DisplayName.IsEmpty() ? MaterialData->DisplayName : FText::FromName(Player->EnhanceMaterialID));
        MaterialNameText->SetColorAndOpacity(FSlateColor(bMax ? Muted : Ivory));
    }

    // 단계 마름모: 지난 단계는 금색, 다음 단계는 테두리만, 나머지는 어둡게
    if (PipShapes.Num() != MaxLevel)
        RebuildPips(MaxLevel);

    for (int32 i = 0; i < PipShapes.Num(); ++i)
    {
        if (!PipShapes[i])
            continue;

        if (i < Level)
            PipShapes[i]->SetBrush(MakeBoxBrush(Gold, GoldBright, 1.f, 0.f));
        else if (i == Level)
            PipShapes[i]->SetBrush(MakeBoxBrush(SRGB(0x3A, 0x2E, 0x18), Gold, 1.5f, 0.f));
        else
            PipShapes[i]->SetBrush(MakeBoxBrush(SRGB(0x22, 0x1B, 0x12), GoldDim, 1.f, 0.f));
    }

    // 버튼 글자가 지금 못 하는 이유를 바로 알려준다
    if (EnhanceLabel)
    {
        FString Text = TEXT("강  화  하  기");

        if (bMax)
            Text = TEXT("최대 강화 완료");
        else if (!bHasMaterial)
            Text = TEXT("재료가 부족합니다");
        else if (!bHasGold)
            Text = TEXT("골드가 부족합니다");

        EnhanceLabel->SetText(FText::FromString(Text));
    }
}

void UUI_Enhance::ShowToast(const FString& Message, bool bGood)
{
    if (!ToastText)
        return;

    ToastText->SetText(FText::FromString(Message));
    ToastText->SetColorAndOpacity(FSlateColor(bGood ? QuestUI::Success : PanelUI::Danger));
    ToastText->SetOpacity(1.f);

    ToastRemaining = ToastSeconds;
}

void UUI_Enhance::ShowHint()
{
    if (!ToastText)
        return;

    ToastText->SetText(FText::FromString(Player
        ? FString::Printf(TEXT("강화할 때마다 공격력이 %d 오릅니다.   [Enter] 강화"), Player->AttackPerEnhance)
        : FString(TEXT("[Enter] 강화"))));
    ToastText->SetColorAndOpacity(FSlateColor(QuestUI::Muted));
    ToastText->SetOpacity(1.f);

    ToastRemaining = 0.f;
}

void UUI_Enhance::HandleInventoryChanged()
{
    RefreshEnhanceUI();
}

void UUI_Enhance::HandleWeaponEnhanced(int32 NewLevel)
{
    FlashRemaining = FlashSeconds;

    ShowToast(FString::Printf(TEXT("강화 성공!   +%d   공격력 +%d"), NewLevel, Player ? Player->AttackPerEnhance : 0), true);

    RefreshEnhanceUI();
}

void UUI_Enhance::OnEnhanceButtonClicked()
{
    if (!Player)
        return;

    if (!Player->TryEnhanceWeapon())
    {
        // 버튼이 비활성일 때는 클릭이 오지 않지만 Enter 키는 올 수 있다. 이유를 알려준다
        CapstoneSFX::Play2D(this, ESFX::UI_Deny);

        if (Player->WeaponLevel >= Player->MaxWeaponLevel)
            ShowToast(TEXT("이미 최대로 강화했습니다."), false);
        else if (Player->InventoryComponent && Player->InventoryComponent->GetItemCount(Player->EnhanceMaterialID) < Player->EnhanceMaterialCount)
            ShowToast(TEXT("강화 재료가 부족합니다."), false);
        else
            ShowToast(TEXT("골드가 부족합니다."), false);
    }
}

void UUI_Enhance::OnCloseButtonClicked()
{
    OnEnhanceClosed.Broadcast();
}
