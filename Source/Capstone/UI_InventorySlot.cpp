#include "UI_InventorySlot.h"
#include "Py_Grey.h"
#include "EquipmentComponent.h"
#include "UI_ItemDrag.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UUI_InventorySlot::InitSlot(const FInventorySlot& SlotData, const FItemData* ItemData)
{
	CurrentItemID = SlotData.ItemID;
	CurrentItemCount = SlotData.Count;

    if (ItemIcon)
    {
        ItemIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

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

        if (IconTexture)
        {
            ItemIcon->SetBrushFromTexture(IconTexture);
        }
    }

    if (APy_Grey* Player = Cast<APy_Grey>(GetOwningPlayerPawn()))
    {
        SetToolTipText(Player->EquipmentComponent ? Player->EquipmentComponent->MakeTooltip(CurrentItemID, false) : FText::GetEmpty());
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
}

bool UUI_InventorySlot::IsEquippableItem() const
{
    if (CurrentItemID.IsNone())
        return false;

    APy_Grey* Player = Cast<APy_Grey>(GetOwningPlayerPawn());
    FEquipInfo Info;

    return Player && Player->EquipmentComponent && Player->EquipmentComponent->GetEquipInfo(CurrentItemID, Info);
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

void UUI_InventorySlot::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);

    bPressedOnEquipment = false;
}

void UUI_InventorySlot::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
    Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

    bPressedOnEquipment = false;

    APy_Grey* Player = Cast<APy_Grey>(GetOwningPlayerPawn());
    FEquipInfo Info;

    if (CurrentItemID.IsNone() || !Player || !Player->EquipmentComponent || !Player->EquipmentComponent->GetEquipInfo(CurrentItemID, Info))
        return;

    UItemDragDropOperation* Operation = NewObject<UItemDragDropOperation>();
    Operation->ItemID = CurrentItemID;
    Operation->FromEquipSlot = EEquipSlot::None;
    Operation->Pivot = EDragPivot::MouseDown;
    Operation->DefaultDragVisual = ItemDrag::MakeVisual(this, Info.Name, Player->EquipmentComponent->ResolveIcon(CurrentItemID));

    OutOperation = Operation;
}

void UUI_InventorySlot::ClearSlot()
{
    CurrentItemID = NAME_None;
    CurrentItemCount = 0;
    bPressedOnEquipment = false;

    SetToolTipText(FText::GetEmpty());

    if (ItemIcon) {
        ItemIcon->SetVisibility(ESlateVisibility::Hidden);
    }

    if (CountText) {
        CountText->SetVisibility(ESlateVisibility::Hidden);
    }
}
