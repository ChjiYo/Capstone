#include "InventoryComponent.h"
#include "Monster.h"
#include "LevelTransitionSubsystem.h"
#include "CapstoneSFX.h"

UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::BeginPlay()
{
    Super::BeginPlay();

    MonsterKilledHandle = AMonster::OnMonsterKilled.AddUObject(this, &UInventoryComponent::HandleMonsterKilled);

    // 포탈을 통해 방금 다른 레벨에서 넘어온 경우, 파괴되기 전 폰이 맡겨둔 인벤토리를 복원한다
    if (UGameInstance* GI = GetWorld()->GetGameInstance())
    {
        if (ULevelTransitionSubsystem* TransitionSubsystem = GI->GetSubsystem<ULevelTransitionSubsystem>())
        {
            TArray<FInventorySlot> SavedItems;
            int32 SavedGold = 0;
            if (TransitionSubsystem->ConsumePendingInventory(SavedItems, SavedGold))
            {
                Items = SavedItems;
                Gold = SavedGold;
                OnInventoryChanged.Broadcast();
            }
        }
    }
}

void UInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    AMonster::OnMonsterKilled.Remove(MonsterKilledHandle);

    Super::EndPlay(EndPlayReason);
}

const FItemData* UInventoryComponent::GetItemData(FName ItemID) const
{
    if (!ItemDataTable)
        return nullptr;

    return ItemDataTable->FindRow<FItemData>(ItemID, TEXT("UInventoryComponent::GetItemData"));
}

void UInventoryComponent::AddItem(FName ItemID, int32 Count)
{
    if (ItemID.IsNone() || Count <= 0)
        return;

    int32 MaxStack = 99;
    if (const FItemData* Data = GetItemData(ItemID))
    {
        MaxStack = Data->MaxStack;
    }

    for (FInventorySlot& Slot : Items)
    {
        if (Slot.ItemID == ItemID)
        {
            Slot.Count = FMath::Min(Slot.Count + Count, MaxStack);

            OnInventoryChanged.Broadcast();

            LOG(TEXT("Item Added : %s, Current Count : %d"), *ItemID.ToString(), Slot.Count);
            return;
        }
    }

    FInventorySlot NewSlot;
    NewSlot.ItemID = ItemID;
    NewSlot.Count = FMath::Min(Count, MaxStack);
    Items.Add(NewSlot);

    OnInventoryChanged.Broadcast();

    LOG(TEXT("Item Added : %s, Current Count : %d"), *ItemID.ToString(), NewSlot.Count);
}

bool UInventoryComponent::RemoveItem(FName ItemID, int32 Count)
{
    if (ItemID.IsNone() || Count <= 0)
        return false;

    for (int32 i = 0; i < Items.Num(); ++i)
    {
        if (Items[i].ItemID != ItemID)
            continue;

        if (Items[i].Count < Count)
            return false;

        Items[i].Count -= Count;

        if (Items[i].Count <= 0)
            Items.RemoveAt(i);

        OnInventoryChanged.Broadcast();
        return true;
    }

    return false;
}

int32 UInventoryComponent::GetItemCount(FName ItemID) const
{
    for (const FInventorySlot& Slot : Items)
    {
        if (Slot.ItemID == ItemID)
            return Slot.Count;
    }

    return 0;
}

void UInventoryComponent::AddGold(int32 Amount)
{
    if (Amount <= 0)
        return;

    Gold += Amount;

    OnInventoryChanged.Broadcast();

    LOG(TEXT("Gold Added : %d, Current Gold : %d"), Amount, Gold);
}

bool UInventoryComponent::TrySpendGold(int32 Amount)
{
    if (Amount <= 0 || Gold < Amount)
        return false;

    Gold -= Amount;

    OnInventoryChanged.Broadcast();

    return true;
}

void UInventoryComponent::HandleMonsterKilled(FName MonsterID, int32 ExpReward, int32 GoldReward)
{
    AddGold(GoldReward);

    // 처치 타격음/사망음이 지나간 뒤에 동전 소리가 들리도록 살짝 늦춘다
    UWorld* World = GetWorld();
    if (GoldReward > 0 && World)
    {
        FTimerHandle CoinTimer;
        World->GetTimerManager().SetTimer(CoinTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
        {
            CapstoneSFX::Play2D(this, ESFX::UI_Coin, 0.7f);
        }), 0.45f, false);
    }
}
