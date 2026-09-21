#include "UI_EquipSlot.h"
#include "UI_ItemDrag.h"
#include "UI_QuestStyle.h"
#include "EquipmentComponent.h"
#include "Py_Grey.h"
#include "CapstoneSFX.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

void UUI_EquipSlot::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// UUserWidget은 기본이 SelfHitTestInvisible이라 자식까지 히트 테스트를 끄면 클릭/끌기를 전혀 받지 못한다
	SetVisibility(ESlateVisibility::Visible);

	if (WidgetTree && !WidgetTree->RootWidget)
		BuildTree();
}

void UUI_EquipSlot::BuildTree()
{
	USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Root"));
	Root->SetWidthOverride(150.f);
	WidgetTree->RootWidget = Root;

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Column->SetVisibility(ESlateVisibility::HitTestInvisible);
	Root->SetContent(Column);

	SlotLabelText = QuestUI::MakeText(WidgetTree, TEXT("SlotLabel"), 13, QuestUI::Gold, true, true, 2);
	SlotLabelText->SetJustification(ETextJustify::Center);

	if (UVerticalBoxSlot* LabelSlot = Column->AddChildToVerticalBox(SlotLabelText))
	{
		LabelSlot->SetHorizontalAlignment(HAlign_Center);
		LabelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}

	USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("IconBox"));
	IconBox->SetWidthOverride(96.f);
	IconBox->SetHeightOverride(96.f);

	Frame = QuestUI::MakeBorder(WidgetTree, TEXT("Frame"), QuestUI::MakeBoxBrush(QuestUI::InsetFill, QuestUI::GoldDim, 2.f, 6.f), FMargin(8.f));
	IconBox->SetContent(Frame);

	UOverlay* IconOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("IconOverlay"));
	Frame->SetContent(IconOverlay);

	IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("IconImage"));
	IconImage->SetVisibility(ESlateVisibility::Collapsed);

	if (UOverlaySlot* IconOverlaySlot = IconOverlay->AddChildToOverlay(IconImage))
	{
		IconOverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		IconOverlaySlot->SetVerticalAlignment(VAlign_Fill);
	}

	EmptyText = QuestUI::MakeText(WidgetTree, TEXT("EmptyText"), 13, QuestUI::Muted, false);
	EmptyText->SetText(FText::FromString(TEXT("비어 있음")));

	if (UOverlaySlot* EmptySlot = IconOverlay->AddChildToOverlay(EmptyText))
	{
		EmptySlot->SetHorizontalAlignment(HAlign_Center);
		EmptySlot->SetVerticalAlignment(VAlign_Center);
	}

	if (UVerticalBoxSlot* IconSlot = Column->AddChildToVerticalBox(IconBox))
		IconSlot->SetHorizontalAlignment(HAlign_Center);

	NameText = QuestUI::MakeText(WidgetTree, TEXT("NameText"), 14, QuestUI::Ivory, true);
	NameText->SetJustification(ETextJustify::Center);
	NameText->SetAutoWrapText(true);

	if (UVerticalBoxSlot* NameSlot = Column->AddChildToVerticalBox(NameText))
	{
		NameSlot->SetHorizontalAlignment(HAlign_Fill);
		NameSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}

	BonusText = QuestUI::MakeText(WidgetTree, TEXT("BonusText"), 12, QuestUI::Success, false);
	BonusText->SetJustification(ETextJustify::Center);
	BonusText->SetAutoWrapText(true);

	if (UVerticalBoxSlot* BonusSlot = Column->AddChildToVerticalBox(BonusText))
	{
		BonusSlot->SetHorizontalAlignment(HAlign_Fill);
		BonusSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
	}
}

void UUI_EquipSlot::SetEquipSlot(EEquipSlot InSlot)
{
	EquipSlot = InSlot;

	if (SlotLabelText)
		SlotLabelText->SetText(EquipDefs::GetSlotLabel(EquipSlot));
}

UEquipmentComponent* UUI_EquipSlot::GetEquipment() const
{
	if (APy_Grey* Player = Cast<APy_Grey>(GetOwningPlayerPawn()))
		return Player->EquipmentComponent;

	return nullptr;
}

void UUI_EquipSlot::Refresh(UEquipmentComponent* Equipment)
{
	CurrentItemID = Equipment ? Equipment->GetEquippedItem(EquipSlot) : NAME_None;

	FEquipInfo Info;
	const bool bHasItem = Equipment && Equipment->GetEquipInfo(CurrentItemID, Info);

	if (bHasItem)
	{
		UTexture2D* Icon = Equipment->ResolveIcon(CurrentItemID);

		if (Icon && IconImage)
		{
			IconImage->SetBrushFromTexture(Icon, false);
			IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else if (IconImage)
		{
			IconImage->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (EmptyText)
			EmptyText->SetVisibility(Icon ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);

		if (NameText)
			NameText->SetText(Info.Name);

		if (BonusText)
		{
			TArray<FString> Parts;

			if (Info.AttackBonus != 0)
				Parts.Add(FString::Printf(TEXT("공격력 %+d"), Info.AttackBonus));

			if (Info.MaxHpBonus != 0)
				Parts.Add(FString::Printf(TEXT("체력 %+d"), Info.MaxHpBonus));

			BonusText->SetText(FText::FromString(FString::Join(Parts, TEXT("  "))));
		}

		SetToolTipText(Equipment->MakeTooltip(CurrentItemID, true));
	}
	else
	{
		if (IconImage)
			IconImage->SetVisibility(ESlateVisibility::Collapsed);

		if (EmptyText)
			EmptyText->SetVisibility(ESlateVisibility::HitTestInvisible);

		if (NameText)
			NameText->SetText(FText::FromString(TEXT("-")));

		if (BonusText)
			BonusText->SetText(FText::GetEmpty());

		SetToolTipText(FText::GetEmpty());
	}

	UpdateFrame();
}

void UUI_EquipSlot::UpdateFrame()
{
	if (!Frame)
		return;

	FLinearColor Outline = QuestUI::GoldDim;

	if (bDropHighlight)
		Outline = QuestUI::Success;
	else if (bHovered && !CurrentItemID.IsNone())
		Outline = QuestUI::GoldBright;
	else if (!CurrentItemID.IsNone())
		Outline = QuestUI::Gold;

	Frame->SetBrush(QuestUI::MakeBoxBrush(QuestUI::InsetFill, Outline, 2.f, 6.f));
}

FReply UUI_EquipSlot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !CurrentItemID.IsNone())
	{
		bPressedOnItem = true;

		// 조금 움직이면 끌기(NativeOnDragDetected), 그대로 떼면 클릭(NativeOnMouseButtonUp)으로 갈라진다
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UUI_EquipSlot::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bPressedOnItem)
	{
		bPressedOnItem = false;

		if (UEquipmentComponent* Equipment = GetEquipment())
			Equipment->Unequip(EquipSlot);

		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UUI_EquipSlot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	bHovered = true;
	UpdateFrame();
}

void UUI_EquipSlot::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	bHovered = false;
	bPressedOnItem = false;
	UpdateFrame();
}

void UUI_EquipSlot::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	bPressedOnItem = false;

	UEquipmentComponent* Equipment = GetEquipment();
	FEquipInfo Info;

	if (CurrentItemID.IsNone() || !Equipment || !Equipment->GetEquipInfo(CurrentItemID, Info))
		return;

	UItemDragDropOperation* Operation = NewObject<UItemDragDropOperation>();
	Operation->ItemID = CurrentItemID;
	Operation->FromEquipSlot = EquipSlot;
	Operation->Pivot = EDragPivot::MouseDown;
	Operation->DefaultDragVisual = ItemDrag::MakeVisual(this, Info.Name, Equipment->ResolveIcon(CurrentItemID));

	OutOperation = Operation;
}

void UUI_EquipSlot::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);

	const UItemDragDropOperation* Operation = Cast<UItemDragDropOperation>(InOperation);
	UEquipmentComponent* Equipment = GetEquipment();
	FEquipInfo Info;

	// 인벤토리에서 끌어온 아이템 중 이 칸에 맞는 것만 초록으로 빛난다
	bDropHighlight = Operation && Equipment && Operation->FromEquipSlot == EEquipSlot::None
		&& Equipment->GetEquipInfo(Operation->ItemID, Info) && Info.Slot == EquipSlot;

	UpdateFrame();
}

void UUI_EquipSlot::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);

	bDropHighlight = false;
	UpdateFrame();
}

bool UUI_EquipSlot::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	bDropHighlight = false;
	UpdateFrame();

	const UItemDragDropOperation* Operation = Cast<UItemDragDropOperation>(InOperation);

	if (!Operation)
		return false;

	// 장비 칸끼리는 주고받을 게 없다
	if (Operation->FromEquipSlot != EEquipSlot::None)
		return false;

	UEquipmentComponent* Equipment = GetEquipment();
	FEquipInfo Info;

	if (!Equipment || !Equipment->GetEquipInfo(Operation->ItemID, Info) || Info.Slot != EquipSlot)
	{
		CapstoneSFX::Play2D(this, ESFX::UI_Deny);
		return true;
	}

	Equipment->EquipFromInventory(Operation->ItemID);
	return true;
}
