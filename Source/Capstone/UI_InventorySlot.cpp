#include "UI_InventorySlot.h"
#include "UI_QuestStyle.h"
#include "Py_Grey.h"
#include "EquipmentComponent.h"
#include "UI_ItemDrag.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

namespace
{
	// 아이템 종류별 테두리 색: 장비/무기는 금색, 물약은 붉은색, 재료는 푸른색, 전리품은 보라색
	FLinearColor AccentFor(const FItemData* Data, bool bEquipment)
	{
		using namespace QuestUI;

		if (bEquipment)
			return Gold;

		if (!Data)
			return Muted;

		switch (Data->ItemType)
		{
		case EItemType::Potion:		return SRGB(0xD8, 0x64, 0x5C);
		case EItemType::Material:	return SRGB(0x6F, 0xA8, 0xD8);
		case EItemType::Loot:		return SRGB(0xAE, 0x88, 0xDC);
		default:					return Gold;
		}
	}

	FText TypeLabelFor(const FItemData* Data)
	{
		if (!Data)
			return FText::GetEmpty();

		switch (Data->ItemType)
		{
		case EItemType::Potion:		return FText::FromString(TEXT("소모품"));
		case EItemType::Material:	return FText::FromString(TEXT("재료"));
		case EItemType::Loot:		return FText::FromString(TEXT("전리품"));
		default:					return FText::FromString(TEXT("무기"));
		}
	}

	// A쪽을 (1-T) 만큼, B쪽을 T 만큼 섞는다. 알파는 1로 둔다
	FLinearColor Mix(const FLinearColor& A, const FLinearColor& B, float T)
	{
		return FLinearColor(FMath::Lerp(A.R, B.R, T), FMath::Lerp(A.G, B.G, T), FMath::Lerp(A.B, B.B, T), 1.f);
	}
}

void UUI_InventorySlot::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 위젯 블루프린트의 트리를 통째로 코드 조립 트리로 교체한다. 디자이너 미리보기가 아니라 실제 인스턴스에서만
	if (bUseCodeBuiltLayout && WidgetTree && !IsDesignTime())
	{
		BuildNativeTree();

		// UUserWidget은 기본이 SelfHitTestInvisible이라 클릭/끌기/마우스 오버를 받으려면 켜 줘야 한다
		SetVisibility(ESlateVisibility::Visible);
	}
}

void UUI_InventorySlot::BuildNativeTree()
{
	using namespace QuestUI;

	USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Root"));
	Root->SetWidthOverride(Size);
	Root->SetHeightOverride(Size);
	WidgetTree->RootWidget = Root;

	Frame = MakeBorder(WidgetTree, TEXT("Frame"), MakeBoxBrush(InsetFill, GoldDim, 2.f, 5.f), FMargin(6.f));
	Root->SetContent(Frame);

	UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Layers"));
	Frame->SetContent(Layers);

	ItemIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ItemIcon"));
	ItemIcon->SetVisibility(ESlateVisibility::Hidden);

	if (UOverlaySlot* IconSlot = Layers->AddChildToOverlay(ItemIcon))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Fill);
		IconSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// 자동 줄바꿈 글자는 가운데 정렬 슬롯에서 임시 글자 폭에서 줄이 바뀌므로 Fill로 두고 글자만 가운데 정렬한다
	FallbackText = MakeText(WidgetTree, TEXT("FallbackText"), 12, Ivory, true);
	FallbackText->SetJustification(ETextJustify::Center);
	FallbackText->SetAutoWrapText(true);
	FallbackText->SetVisibility(ESlateVisibility::Collapsed);

	if (UOverlaySlot* FallbackSlot = Layers->AddChildToOverlay(FallbackText))
	{
		FallbackSlot->SetHorizontalAlignment(HAlign_Fill);
		FallbackSlot->SetVerticalAlignment(VAlign_Center);
	}

	CountText = MakeText(WidgetTree, TEXT("CountText"), 14, GoldBright, true);
	CountText->SetVisibility(ESlateVisibility::Hidden);

	if (UOverlaySlot* CountSlot = Layers->AddChildToOverlay(CountText))
	{
		CountSlot->SetHorizontalAlignment(HAlign_Right);
		CountSlot->SetVerticalAlignment(VAlign_Bottom);
		CountSlot->SetPadding(FMargin(0.f, 0.f, -2.f, -3.f));
	}
}

void UUI_InventorySlot::UpdateFrame()
{
	if (!Frame)
		return;

	using namespace QuestUI;

	FLinearColor Fill = SRGB(0x15, 0x11, 0x0C);
	FLinearColor Outline = SRGB(0x5E, 0x4B, 0x2E);
	float Width = 1.5f;

	if (HasItem())
	{
		Fill = Mix(SRGB(0x24, 0x1C, 0x12), Info.Accent, bHovered ? 0.22f : 0.12f);
		Outline = bHovered ? Mix(Info.Accent, FLinearColor::White, 0.4f) : Info.Accent;
		Width = 2.f;
	}
	else if (bHovered)
	{
		Outline = GoldDim;
	}

	Frame->SetBrush(MakeBoxBrush(Fill, Outline, Width, 5.f));
}

void UUI_InventorySlot::SetIconTexture(UTexture2D* Texture, const FText& FallbackName)
{
	if (ItemIcon)
	{
		if (Texture)
		{
			ItemIcon->SetBrushFromTexture(Texture);
			ItemIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			// 코드 조립 칸은 아이콘 대신 이름 글자를 보여준다. WBP 칸은 예전처럼 아이콘 위젯을 그대로 둔다
			ItemIcon->SetVisibility(FallbackText ? ESlateVisibility::Hidden : ESlateVisibility::SelfHitTestInvisible);
		}
	}

	if (FallbackText)
	{
		FallbackText->SetText(FallbackName);
		FallbackText->SetVisibility(Texture ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

FInventorySlotInfo UUI_InventorySlot::BuildInfo(const FItemData* ItemData) const
{
	FInventorySlotInfo Result;

	APy_Grey* Player = Cast<APy_Grey>(GetOwningPlayerPawn());
	FEquipInfo Equip;

	if (Player && Player->EquipmentComponent && Player->EquipmentComponent->GetEquipInfo(CurrentItemID, Equip))
	{
		Result.Accent = AccentFor(ItemData, true);
		Result.Name = Equip.Name;
		Result.Type = FText::FromString(FString::Printf(TEXT("장비  ·  %s"), *EquipDefs::GetSlotLabel(Equip.Slot).ToString()));

		TArray<FString> Lines;

		if (!Equip.Description.IsEmpty())
			Lines.Add(Equip.Description.ToString());

		TArray<FString> Bonuses;

		if (Equip.AttackBonus != 0)
			Bonuses.Add(FString::Printf(TEXT("공격력 %+d"), Equip.AttackBonus));

		if (Equip.MaxHpBonus != 0)
			Bonuses.Add(FString::Printf(TEXT("최대 체력 %+d"), Equip.MaxHpBonus));

		if (Bonuses.Num() > 0)
			Lines.Add(FString::Join(Bonuses, TEXT("    ")));

		Lines.Add(TEXT("클릭하거나 끌어다 놓아 장착"));

		Result.Body = FText::FromString(FString::Join(Lines, TEXT("\n")));
		return Result;
	}

	Result.Accent = AccentFor(ItemData, false);
	Result.Name = ItemData && !ItemData->DisplayName.IsEmpty() ? ItemData->DisplayName : FText::FromName(CurrentItemID);
	Result.Type = TypeLabelFor(ItemData);

	TArray<FString> Lines;

	if (ItemData && ItemData->HealAmount > 0)
		Lines.Add(FString::Printf(TEXT("체력 %d 회복"), ItemData->HealAmount));

	if (ItemData && ItemData->SellPrice > 0)
		Lines.Add(FString::Printf(TEXT("판매가 %d G"), ItemData->SellPrice));

	if (ItemData && ItemData->ItemType == EItemType::Potion)
		Lines.Add(TEXT("클릭하면 사용합니다"));

	Result.Body = FText::FromString(FString::Join(Lines, TEXT("\n")));
	return Result;
}

void UUI_InventorySlot::InitSlot(const FInventorySlot& SlotData, const FItemData* ItemData)
{
	CurrentItemID = SlotData.ItemID;
	CurrentItemCount = SlotData.Count;

	Info = BuildInfo(ItemData);

	UTexture2D* IconTexture = nullptr;

	if (ItemData && !ItemData->Icon.IsNull())
	{
		IconTexture = ItemData->Icon.LoadSynchronous();
	}

	// DataTable에 아이콘이 없는 장비(기본 검/방패 등)는 코드로 그린 아이콘을 쓴다
	if (!IconTexture)
	{
		IconTexture = EquipDefs::GetGeneratedIcon(CurrentItemID);
	}

	SetIconTexture(IconTexture, Info.Name);

	// 코드 조립 인벤토리는 아래쪽 설명 칸이 이 역할을 대신한다
	if (!bUseCodeBuiltLayout)
	{
		if (APy_Grey* Player = Cast<APy_Grey>(GetOwningPlayerPawn()))
		{
			SetToolTipText(Player->EquipmentComponent ? Player->EquipmentComponent->MakeTooltip(CurrentItemID, false) : FText::GetEmpty());
		}
	}

	if (CountText)
	{
		if (CurrentItemCount > 1)
		{
			CountText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			CountText->SetText(FText::AsNumber(CurrentItemCount));
		}
		else
		{
			CountText->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	UpdateFrame();
}

bool UUI_InventorySlot::IsEquippableItem() const
{
	if (CurrentItemID.IsNone())
		return false;

	APy_Grey* Player = Cast<APy_Grey>(GetOwningPlayerPawn());
	FEquipInfo EquipInfo;

	return Player && Player->EquipmentComponent && Player->EquipmentComponent->GetEquipInfo(CurrentItemID, EquipInfo);
}

FReply UUI_InventorySlot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && IsEquippableItem())
	{
		bPressedOnEquipment = true;

		// 조금 움직이면 끌기(NativeOnDragDetected), 그대로 떼면 클릭(NativeOnMouseButtonUp)으로 갈라진다
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !CurrentItemID.IsNone())
	{
		if (APy_Grey* Player = Cast<APy_Grey>(GetOwningPlayerPawn()))
		{
			Player->UsePotionItem(CurrentItemID);
		}

		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UUI_InventorySlot::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bPressedOnEquipment)
	{
		bPressedOnEquipment = false;

		if (APy_Grey* Player = Cast<APy_Grey>(GetOwningPlayerPawn()))
		{
			if (Player->EquipmentComponent)
				Player->EquipmentComponent->EquipFromInventory(CurrentItemID);
		}

		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UUI_InventorySlot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	bHovered = true;
	UpdateFrame();
	OnSlotHover.Broadcast(this, true);
}

void UUI_InventorySlot::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	bPressedOnEquipment = false;
	bHovered = false;
	UpdateFrame();
	OnSlotHover.Broadcast(this, false);
}

void UUI_InventorySlot::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	bPressedOnEquipment = false;

	APy_Grey* Player = Cast<APy_Grey>(GetOwningPlayerPawn());
	FEquipInfo EquipInfo;

	if (CurrentItemID.IsNone() || !Player || !Player->EquipmentComponent || !Player->EquipmentComponent->GetEquipInfo(CurrentItemID, EquipInfo))
		return;

	UItemDragDropOperation* Operation = NewObject<UItemDragDropOperation>();
	Operation->ItemID = CurrentItemID;
	Operation->FromEquipSlot = EEquipSlot::None;
	Operation->Pivot = EDragPivot::MouseDown;
	Operation->DefaultDragVisual = ItemDrag::MakeVisual(this, EquipInfo.Name, Player->EquipmentComponent->ResolveIcon(CurrentItemID));

	OutOperation = Operation;
}

void UUI_InventorySlot::ClearSlot()
{
	CurrentItemID = NAME_None;
	CurrentItemCount = 0;
	bPressedOnEquipment = false;
	Info = FInventorySlotInfo();

	SetToolTipText(FText::GetEmpty());

	if (ItemIcon) {
		ItemIcon->SetVisibility(ESlateVisibility::Hidden);
	}

	if (FallbackText) {
		FallbackText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (CountText) {
		CountText->SetVisibility(ESlateVisibility::Hidden);
	}

	UpdateFrame();
}
