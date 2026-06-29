#include "UI_InventorySlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UUI_InventorySlot::InitSlot(FName InItemID)
{
	CurrentItemID = InItemID;

    if (ItemIcon)
        ItemIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UUI_InventorySlot::ClearSlot()
{
    CurrentItemID = NAME_None;

    if (ItemIcon) {
        ItemIcon->SetVisibility(ESlateVisibility::Hidden);
    }
}
