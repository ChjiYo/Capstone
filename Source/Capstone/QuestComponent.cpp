#include "QuestComponent.h"
#include "Monster.h"
#include "InventoryComponent.h"
#include "ExperienceComponent.h"

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

void UQuestComponent::HandleMonsterKilled(FName MonsterID, int32 ExpReward, int32 GoldReward)
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
	AActor* Owner = GetOwner();
	if (!Owner)
		return;

	UInventoryComponent* Inventory = Owner->FindComponentByClass<UInventoryComponent>();

	if (Inventory && !ActiveQuest.Reward.RewardItemID.IsNone())
	{
		Inventory->AddItem(ActiveQuest.Reward.RewardItemID, ActiveQuest.Reward.RewardItemCount);
	}

	if (Inventory && ActiveQuest.Reward.RewardGold > 0)
	{
		Inventory->AddGold(ActiveQuest.Reward.RewardGold);
	}

	if (ActiveQuest.Reward.RewardExp > 0)
	{
		if (UExperienceComponent* Experience = Owner->FindComponentByClass<UExperienceComponent>())
		{
			Experience->AddExp(ActiveQuest.Reward.RewardExp);
		}
	}
}
