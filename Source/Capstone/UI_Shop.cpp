#include "UI_Shop.h"
#include "UI_ShopSlot.h"
#include "InventoryComponent.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UUI_Shop::InitShop(const TArray<FName>& InShopItemIDs, UInventoryComponent* InInventoryComp)
{
    ShopItemIDs = InShopItemIDs;
    InventoryComp = InInventoryComp;

    if (InventoryComp)
    {
        InventoryComp->OnInventoryChanged.AddDynamic(this, &UUI_Shop::HandleInventoryChanged);
    }

    RefreshShopUI();
}

void UUI_Shop::NativeConstruct()
{
    Super::NativeConstruct();

    if (CloseButton)
    {
        CloseButton->OnClicked.AddDynamic(this, &UUI_Shop::OnCloseButtonClicked);
    }
}

void UUI_Shop::NativeDestruct()
{
    if (InventoryComp)
    {
        InventoryComp->OnInventoryChanged.RemoveDynamic(this, &UUI_Shop::HandleInventoryChanged);
    }

    Super::NativeDestruct();
}

void UUI_Shop::RefreshShopUI()
{
    if (!InventoryComp)
        return;

    if (BuyItemsPanel && ShopSlotClass)
    {
        BuyItemsPanel->ClearChildren();

        for (const FName& ItemID : ShopItemIDs)
        {
            const FItemData* ItemData = InventoryComp->GetItemData(ItemID);
            if (!ItemData)
                continue;

            UUI_ShopSlot* ShopSlot = CreateWidget<UUI_ShopSlot>(GetWorld(), ShopSlotClass);
            if (ShopSlot)
            {
                ShopSlot->InitSlot(ItemID, ItemData, 0, EShopSlotMode::Buy, InventoryComp);
                BuyItemsPanel->AddChild(ShopSlot);
            }
        }
    }

    if (SellItemsPanel && ShopSlotClass)
    {
        SellItemsPanel->ClearChildren();

        for (const FInventorySlot& InvSlot : InventoryComp->Items)
        {
            const FItemData* ItemData = InventoryComp->GetItemData(InvSlot.ItemID);
            if (!ItemData || ItemData->SellPrice <= 0)
                continue;

            UUI_ShopSlot* ShopSlot = CreateWidget<UUI_ShopSlot>(GetWorld(), ShopSlotClass);
            if (ShopSlot)
            {
                ShopSlot->InitSlot(InvSlot.ItemID, ItemData, InvSlot.Count, EShopSlotMode::Sell, InventoryComp);
                SellItemsPanel->AddChild(ShopSlot);
            }
        }
    }

    if (GoldText)
    {
        GoldText->SetText(FText::AsNumber(InventoryComp->GetGold()));
    }
}

void UUI_Shop::HandleInventoryChanged()
{
    RefreshShopUI();
}

void UUI_Shop::OnCloseButtonClicked()
{
    OnShopClosed.Broadcast();
}
