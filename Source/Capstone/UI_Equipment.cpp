#include "UI_Equipment.h"
#include "UI_EquipSlot.h"
#include "UI_QuestStyle.h"
#include "EquipmentComponent.h"
#include "Py_Grey.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UUI_Equipment::InitEquipment(APy_Grey* InPlayer)
{
	Player = InPlayer;

	RefreshSlots();
	RefreshStats(true);
}

void UUI_Equipment::PlaceInViewport()
{
	// SetPositionInViewport는 앵커를 (0,0)으로 되돌리므로 쓰지 않는다. 오프셋이 0이면 앵커 위치에 알아서 자동 크기로 놓인다
	SetAnchorsInViewport(FAnchors(0.2f, 0.5f));
	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
}

void UUI_Equipment::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree && !WidgetTree->RootWidget)
		BuildTree();

	if (CloseButton)
		CloseButton->OnClicked.AddDynamic(this, &UUI_Equipment::HandleCloseClicked);
}

void UUI_Equipment::NativeConstruct()
{
	Super::NativeConstruct();

	// 뷰포트에 올라올 때마다 구독한다 (NativeDestruct가 창을 닫을 때 구독을 풀기 때문)
	if (Player && Player->EquipmentComponent)
		Player->EquipmentComponent->OnEquipmentChanged.AddUniqueDynamic(this, &UUI_Equipment::HandleEquipmentChanged);

	RefreshSlots();
	RefreshStats(true);
}

void UUI_Equipment::NativeDestruct()
{
	if (Player && Player->EquipmentComponent)
		Player->EquipmentComponent->OnEquipmentChanged.RemoveDynamic(this, &UUI_Equipment::HandleEquipmentChanged);

	Super::NativeDestruct();
}

void UUI_Equipment::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 체력은 피격/회복으로, 공격력은 레벨업/강화/스킬 버프로 수시로 바뀐다
	RefreshStats(false);
}

void UUI_Equipment::HandleEquipmentChanged()
{
	RefreshSlots();
	RefreshStats(true);
}

void UUI_Equipment::HandleCloseClicked()
{
	OnCloseRequested.Broadcast();
}

void UUI_Equipment::BuildTree()
{
	using namespace QuestUI;

	// 바깥 프레임: 어두운 판 + 금색 테두리
	UBorder* Outer = MakeBorder(WidgetTree, TEXT("Outer"), MakeBoxBrush(PanelFill, Gold, 2.f, 10.f), FMargin(3.f));
	WidgetTree->RootWidget = Outer;

	UBorder* Inner = MakeBorder(WidgetTree, TEXT("Inner"), MakeBoxBrush(PanelInner, GoldDim, 1.f, 8.f), FMargin(22.f, 16.f, 22.f, 18.f));
	Outer->SetContent(Inner);

	USizeBox* WidthBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("WidthBox"));
	WidthBox->SetWidthOverride(380.f);
	Inner->SetContent(WidthBox);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	WidthBox->SetContent(Column);

	// ---- 제목줄: 마름모 + 제목 + 닫기 버튼 ----
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Header"));

	if (UHorizontalBoxSlot* DiamondSlot = Header->AddChildToHorizontalBox(MakeDiamond(WidgetTree, TEXT("TitleDiamond"), Gold, 9.f)))
	{
		DiamondSlot->SetVerticalAlignment(VAlign_Center);
		DiamondSlot->SetPadding(FMargin(4.f, 0.f, 10.f, 0.f));
	}

	UTextBlock* Title = MakeText(WidgetTree, TEXT("Title"), 22, GoldBright, true, true, 6);
	Title->SetText(FText::FromString(TEXT("장  비")));

	if (UHorizontalBoxSlot* TitleSlot = Header->AddChildToHorizontalBox(Title))
	{
		TitleSlot->SetVerticalAlignment(VAlign_Center);
		TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UTextBlock* KeyHint = MakeText(WidgetTree, TEXT("KeyHint"), 12, Muted, false);
	KeyHint->SetText(FText::FromString(TEXT("[U]")));

	if (UHorizontalBoxSlot* HintSlot = Header->AddChildToHorizontalBox(KeyHint))
	{
		HintSlot->SetVerticalAlignment(VAlign_Center);
		HintSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	}

	CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
	CloseButton->SetStyle(MakeButtonStyle(InsetFill, PanelInner, InsetFill, GoldDim, Gold));

	UTextBlock* CloseLabel = MakeText(WidgetTree, TEXT("CloseLabel"), 14, Ivory, true);
	CloseLabel->SetText(FText::FromString(TEXT("X")));
	CloseLabel->SetJustification(ETextJustify::Center);
	CloseButton->SetContent(CloseLabel);

	USizeBox* CloseBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CloseBox"));
	CloseBox->SetWidthOverride(28.f);
	CloseBox->SetHeightOverride(28.f);
	CloseBox->SetContent(CloseButton);

	if (UHorizontalBoxSlot* CloseSlot = Header->AddChildToHorizontalBox(CloseBox))
		CloseSlot->SetVerticalAlignment(VAlign_Center);

	Column->AddChildToVerticalBox(Header);

	if (UVerticalBoxSlot* LineSlot = Column->AddChildToVerticalBox(MakeLine(WidgetTree, TEXT("TitleLine"), Gold, 1.5f, true)))
		LineSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 18.f));

	// ---- 장비 칸: 무기 / 방패 ----
	UHorizontalBox* Slots = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Slots"));

	WeaponSlotWidget = CreateWidget<UUI_EquipSlot>(this);
	WeaponSlotWidget->SetEquipSlot(EEquipSlot::Weapon);

	if (UHorizontalBoxSlot* WeaponBoxSlot = Slots->AddChildToHorizontalBox(WeaponSlotWidget))
	{
		WeaponBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		WeaponBoxSlot->SetHorizontalAlignment(HAlign_Center);
	}

	ShieldSlotWidget = CreateWidget<UUI_EquipSlot>(this);
	ShieldSlotWidget->SetEquipSlot(EEquipSlot::Shield);

	if (UHorizontalBoxSlot* ShieldBoxSlot = Slots->AddChildToHorizontalBox(ShieldSlotWidget))
	{
		ShieldBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ShieldBoxSlot->SetHorizontalAlignment(HAlign_Center);
	}

	Column->AddChildToVerticalBox(Slots);

	if (UVerticalBoxSlot* LineSlot2 = Column->AddChildToVerticalBox(MakeLine(WidgetTree, TEXT("StatLine"), GoldDim, 1.f, true)))
		LineSlot2->SetPadding(FMargin(0.f, 18.f, 0.f, 12.f));

	// ---- 능력치 ----
	auto AddStatRow = [&](const TCHAR* Name, const TCHAR* Label, TObjectPtr<UTextBlock>& OutValue, TObjectPtr<UTextBlock>& OutDetail)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), FName(*FString::Printf(TEXT("%sRow"), Name)));

		UTextBlock* LabelText = MakeText(WidgetTree, *FString::Printf(TEXT("%sLabel"), Name), 16, Gold, true, true, 2);
		LabelText->SetText(FText::FromString(Label));

		if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText))
		{
			LabelSlot->SetVerticalAlignment(VAlign_Center);
			LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		OutValue = MakeText(WidgetTree, *FString::Printf(TEXT("%sValue"), Name), 22, Ivory, true);

		if (UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(OutValue))
			ValueSlot->SetVerticalAlignment(VAlign_Center);

		Column->AddChildToVerticalBox(Row);

		OutDetail = MakeText(WidgetTree, *FString::Printf(TEXT("%sDetail"), Name), 12, Muted, false);
		OutDetail->SetJustification(ETextJustify::Right);

		if (UVerticalBoxSlot* DetailSlot = Column->AddChildToVerticalBox(OutDetail))
		{
			DetailSlot->SetHorizontalAlignment(HAlign_Right);
			DetailSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
		}
	};

	AddStatRow(TEXT("Attack"), TEXT("공격력"), AttackValueText, AttackDetailText);
	AddStatRow(TEXT("Hp"), TEXT("체력"), HpValueText, HpDetailText);

	if (UVerticalBoxSlot* LineSlot3 = Column->AddChildToVerticalBox(MakeLine(WidgetTree, TEXT("FootLine"), GoldDim, 1.f, true)))
		LineSlot3->SetPadding(FMargin(0.f, 2.f, 0.f, 10.f));

	UTextBlock* Footer = MakeText(WidgetTree, TEXT("Footer"), 12, Muted, false);
	Footer->SetText(FText::FromString(TEXT("인벤토리의 장비를 클릭하거나 이 창으로 끌어다 놓으면 장착하고,\n장착 칸을 클릭하거나 인벤토리로 끌어다 놓으면 해제합니다.")));
	Footer->SetJustification(ETextJustify::Center);
	Footer->SetAutoWrapText(true);

	if (UVerticalBoxSlot* FooterSlot = Column->AddChildToVerticalBox(Footer))
		FooterSlot->SetHorizontalAlignment(HAlign_Center);
}

void UUI_Equipment::RefreshSlots()
{
	UEquipmentComponent* Equipment = Player ? Player->EquipmentComponent : nullptr;

	if (WeaponSlotWidget)
		WeaponSlotWidget->Refresh(Equipment);

	if (ShieldSlotWidget)
		ShieldSlotWidget->Refresh(Equipment);
}

void UUI_Equipment::RefreshStats(bool bForce)
{
	if (!Player)
		return;

	UEquipmentComponent* Equipment = Player->EquipmentComponent;

	const int32 Attack = Player->GetAttackPower();
	const int32 EffectiveAttack = FMath::RoundToInt(Player->GetEffectiveAttackPower());
	const int32 AttackBonus = Equipment ? Equipment->GetTotalAttackBonus() : 0;
	const int32 Hp = Player->GetCurrentHp();
	const int32 MaxHp = Player->GetMaxHp();
	const int32 HpBonus = Equipment ? Equipment->GetTotalMaxHpBonus() : 0;

	const bool bAttackChanged = bForce || Attack != ShownAttack || EffectiveAttack != ShownEffectiveAttack || AttackBonus != ShownAttackBonus;
	const bool bHpChanged = bForce || Hp != ShownHp || MaxHp != ShownMaxHp || HpBonus != ShownHpBonus;

	if (bAttackChanged)
	{
		ShownAttack = Attack;
		ShownEffectiveAttack = EffectiveAttack;
		ShownAttackBonus = AttackBonus;

		if (AttackValueText)
			AttackValueText->SetText(FText::AsNumber(Attack));

		if (AttackDetailText)
		{
			FString Detail = FString::Printf(TEXT("기본 %d + 장비 %d"), Attack - AttackBonus, AttackBonus);

			// 스킬 A로 공격력이 강화된 동안은 실제 타격에 쓰이는 값을 함께 보여준다
			if (Player->IsAttackBuffActive())
				Detail += FString::Printf(TEXT("   (강화 중: %d)"), EffectiveAttack);

			AttackDetailText->SetText(FText::FromString(Detail));
			AttackDetailText->SetColorAndOpacity(FSlateColor(Player->IsAttackBuffActive() ? QuestUI::Success : QuestUI::Muted));
		}
	}

	if (bHpChanged)
	{
		ShownHp = Hp;
		ShownMaxHp = MaxHp;
		ShownHpBonus = HpBonus;

		if (HpValueText)
			HpValueText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Hp, MaxHp)));

		if (HpDetailText)
			HpDetailText->SetText(FText::FromString(FString::Printf(TEXT("기본 최대 %d + 장비 %d"), MaxHp - HpBonus, HpBonus)));
	}
}
