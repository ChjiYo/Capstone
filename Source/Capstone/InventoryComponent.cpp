#include "InventoryComponent.h"

UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::AddItem(FName NewItemID)
{
    Items.Add(NewItemID);

    LOG(TEXT("Item Added : %s"), *NewItemID.ToString());
    LOG(TEXT("Current Item Count : %d"), Items.Num());
}

