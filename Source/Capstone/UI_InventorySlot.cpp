#include "UI_InventorySlot.h"
#include "Py_Grey.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UUI_InventorySlot::InitSlot(const FInventorySlot& SlotData, const FItemData* ItemData)
{
	CurrentItemID = SlotData.ItemID;
	CurrentItemCount = SlotData.Count;

    if (ItemIcon)
    {
        ItemIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

        if (ItemData && !ItemData->Icon.IsNull())
        {
            if (UTexture2D* IconTexture = ItemData->Icon.LoadSynchronous())
            {
                ItemIcon->SetBrushFromTexture(IconTexture);
            }
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
}

FReply UUI_InventorySlot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
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

void UUI_InventorySlot::ClearSlot()
{
    CurrentItemID = NAME_None;
    CurrentItemCount = 0;

    if (ItemIcon) {
        ItemIcon->SetVisibility(ESlateVisibility::Hidden);
    }

    if (CountText) {
        CountText->SetVisibility(ESlateVisibility::Hidden);
    }
}
