#include "UI_ShopSlot.h"
#include "InventoryComponent.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UUI_ShopSlot::InitSlot(FName InItemID, const FItemData* ItemData, int32 OwnedCount, EShopSlotMode InMode, UInventoryComponent* InInventoryComp)
{
    CurrentItemID = InItemID;
    CurrentMode = InMode;
    InventoryComp = InInventoryComp;
    CurrentPrice = ItemData ? (InMode == EShopSlotMode::Buy ? ItemData->BuyPrice : ItemData->SellPrice) : 0;

    if (ItemIcon && ItemData && !ItemData->Icon.IsNull())
    {
        if (UTexture2D* IconTexture = ItemData->Icon.LoadSynchronous())
        {
            ItemIcon->SetBrushFromTexture(IconTexture);
        }
    }

    if (NameText)
    {
        NameText->SetText(ItemData ? ItemData->DisplayName : FText::FromName(CurrentItemID));
    }

    if (PriceText)
    {
        PriceText->SetText(FText::AsNumber(CurrentPrice));
    }

    if (CountText)
    {
        if (CurrentMode == EShopSlotMode::Sell)
        {
            CountText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
            CountText->SetText(FText::AsNumber(OwnedCount));
        }
        else
        {
            CountText->SetVisibility(ESlateVisibility::Hidden);
        }
    }

    if (ActionButton)
    {
        ActionButton->OnClicked.RemoveAll(this);
        ActionButton->OnClicked.AddDynamic(this, &UUI_ShopSlot::OnActionButtonClicked);
    }
}

void UUI_ShopSlot::OnActionButtonClicked()
{
    if (!InventoryComp)
        return;

    if (CurrentMode == EShopSlotMode::Buy)
    {
        if (InventoryComp->TrySpendGold(CurrentPrice))
        {
            InventoryComp->AddItem(CurrentItemID, 1);
        }
    }
    else
    {
        if (InventoryComp->RemoveItem(CurrentItemID, 1))
        {
            InventoryComp->AddGold(CurrentPrice);
        }
    }
}
