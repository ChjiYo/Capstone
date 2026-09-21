#include "InventoryComponent.h"
#include "Monster.h"
#include "LevelTransitionSubsystem.h"
#include "CapstoneSFX.h"
#include "EquipmentData.h"

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

const FItemData* UInventoryComponent::FindTableRow(FName ItemID) const
{
    if (!ItemDataTable)
        return nullptr;

    return ItemDataTable->FindRow<FItemData>(ItemID, TEXT("UInventoryComponent::FindTableRow"));
}

const FItemData* UInventoryComponent::GetItemData(FName ItemID) const
{
    if (const FItemData* Row = FindTableRow(ItemID))
        return Row;

    if (const FItemData* Cached = SyntheticItems.Find(ItemID))
        return Cached;

    // 표에 없더라도 코드에 정의된 장비(기본 검/방패 등)는 인벤토리에서 이름과 아이콘이 나오도록 행을 만들어 준다
    if (const FEquipInfo* Def = EquipDefs::FindDefinition(ItemID))
    {
        FItemData Data;
        Data.DisplayName = Def->Name;
        Data.ItemType = EItemType::Weapon;
        Data.MaxStack = 1;
        Data.EquipSlot = Def->Slot;
        Data.AttackBonus = Def->AttackBonus;
        Data.MaxHpBonus = Def->MaxHpBonus;

        return &SyntheticItems.Add(ItemID, Data);
    }

    return nullptr;
}

bool UInventoryComponent::CanAddItem(FName ItemID, int32 Count) const
{
    if (ItemID.IsNone() || Count <= 0)
        return false;

    int32 MaxStack = 99;
    if (const FItemData* Data = GetItemData(ItemID))
        MaxStack = FMath::Max(1, Data->MaxStack);

    int32 Remaining = Count;
    for (const FInventorySlot& Slot : Items)
    {
        if (Slot.ItemID == ItemID)
            Remaining -= FMath::Max(0, MaxStack - Slot.Count);
    }

    if (Remaining <= 0)
        return true;

    const int32 NeededSlots = FMath::DivideAndRoundUp(Remaining, MaxStack);
    return Items.Num() + NeededSlots <= MaxSlots;
}

void UInventoryComponent::AddItem(FName ItemID, int32 Count)
{
    if (ItemID.IsNone() || Count <= 0)
        return;

    int32 MaxStack = 99;
    if (const FItemData* Data = GetItemData(ItemID))
    {
        MaxStack = FMath::Max(1, Data->MaxStack);
    }

    // 기존 스택의 남은 자리부터 채우고, 넘치는 만큼은 새 스택으로 만든다 (장비처럼 MaxStack이 1인 아이템을 여러 개 들 수 있게)
    int32 Remaining = Count;

    for (FInventorySlot& Slot : Items)
    {
        if (Remaining <= 0)
            break;

        if (Slot.ItemID != ItemID || Slot.Count >= MaxStack)
            continue;

        const int32 Added = FMath::Min(MaxStack - Slot.Count, Remaining);
        Slot.Count += Added;
        Remaining -= Added;
    }

    while (Remaining > 0)
    {
        FInventorySlot NewSlot;
        NewSlot.ItemID = ItemID;
        NewSlot.Count = FMath::Min(Remaining, MaxStack);
        Items.Add(NewSlot);
        Remaining -= NewSlot.Count;
    }

    OnInventoryChanged.Broadcast();

    LOG(TEXT("Item Added : %s, Current Count : %d"), *ItemID.ToString(), GetItemCount(ItemID));
}

bool UInventoryComponent::RemoveItem(FName ItemID, int32 Count)
{
    if (ItemID.IsNone() || Count <= 0)
        return false;

    if (GetItemCount(ItemID) < Count)
        return false;

    // 여러 스택에 나뉘어 있을 수 있으므로 뒤쪽 스택부터 비워 나간다
    int32 Remaining = Count;

    for (int32 i = Items.Num() - 1; i >= 0 && Remaining > 0; --i)
    {
        if (Items[i].ItemID != ItemID)
            continue;

        const int32 Taken = FMath::Min(Items[i].Count, Remaining);
        Items[i].Count -= Taken;
        Remaining -= Taken;

        if (Items[i].Count <= 0)
            Items.RemoveAt(i);
    }

    OnInventoryChanged.Broadcast();
    return true;
}

int32 UInventoryComponent::GetItemCount(FName ItemID) const
{
    int32 Total = 0;

    for (const FInventorySlot& Slot : Items)
    {
        if (Slot.ItemID == ItemID)
            Total += Slot.Count;
    }

    return Total;
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
