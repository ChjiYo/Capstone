#include "InventoryComponent.h"
#include "Monster.h"

UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::BeginPlay()
{
    Super::BeginPlay();

    MonsterKilledHandle = AMonster::OnMonsterKilled.AddUObject(this, &UInventoryComponent::HandleMonsterKilled);
}

void UInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    AMonster::OnMonsterKilled.Remove(MonsterKilledHandle);

    Super::EndPlay(EndPlayReason);
}

void UInventoryComponent::AddItem(FName NewItemID)
{
    Items.Add(NewItemID);

    LOG(TEXT("Item Added : %s"), *NewItemID.ToString());
    LOG(TEXT("Current Item Count : %d"), Items.Num());
}

void UInventoryComponent::AddGold(int32 Amount)
{
    if (Amount <= 0)
        return;

    Gold += Amount;

    LOG(TEXT("Gold Added : %d, Current Gold : %d"), Amount, Gold);
}

void UInventoryComponent::HandleMonsterKilled(FName MonsterID, int32 ExpReward, int32 GoldReward)
{
    AddGold(GoldReward);
}

