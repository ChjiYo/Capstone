#include "UI_InventorySlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UUI_InventorySlot::InitSlot(const FInventorySlot& SlotData)
{
	CurrentItemID = SlotData.ItemID;
	CurrentItemCount = SlotData.Count;

    if (ItemIcon)
        ItemIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

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
