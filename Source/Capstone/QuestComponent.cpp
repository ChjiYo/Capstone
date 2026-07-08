#include "QuestComponent.h"
#include "Monster.h"
#include "InventoryComponent.h"

UQuestComponent::UQuestComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UQuestComponent::BeginPlay()
{
	Super::BeginPlay();

	MonsterKilledHandle = AMonster::OnMonsterKilled.AddUObject(this, &UQuestComponent::HandleMonsterKilled);
}

void UQuestComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AMonster::OnMonsterKilled.Remove(MonsterKilledHandle);

	Super::EndPlay(EndPlayReason);
}

void UQuestComponent::AcceptQuest(const FQuestData& InQuestData)
{
	ActiveQuest = InQuestData;
	ActiveQuest.CurrentKillCount = 0;
	ActiveQuest.bIsCompleted = false;
	bHasActiveQuest = true;

	OnQuestTrackerChanged.Broadcast();
}

void UQuestComponent::HandleMonsterKilled(FName MonsterID)
{
	if (!bHasActiveQuest || ActiveQuest.bIsCompleted)
		return;

	if (!ActiveQuest.HasKillObjective() || ActiveQuest.KillTargetID.IsNone() || ActiveQuest.KillTargetID != MonsterID)
		return;

	ActiveQuest.CurrentKillCount = FMath::Min(ActiveQuest.CurrentKillCount + 1, ActiveQuest.KillTargetCount);

	if (ActiveQuest.CurrentKillCount >= ActiveQuest.KillTargetCount)
	{
		ActiveQuest.bIsCompleted = true;
		GrantReward();
	}

	OnQuestTrackerChanged.Broadcast();
}

void UQuestComponent::GrantReward()
{
	if (ActiveQuest.Reward.RewardItemID.IsNone())
		return;

	UInventoryComponent* Inventory = GetOwner() ? GetOwner()->FindComponentByClass<UInventoryComponent>() : nullptr;
	if (!Inventory)
		return;

	for (int32 i = 0; i < ActiveQuest.Reward.RewardItemCount; ++i)
	{
		Inventory->AddItem(ActiveQuest.Reward.RewardItemID);
	}
}
