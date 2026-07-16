#include "QuestComponent.h"
#include "Monster.h"
#include "GatherableObject.h"
#include "Py_Grey.h"
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
	ItemGatheredHandle = AGatherableObject::OnItemGathered.AddUObject(this, &UQuestComponent::HandleItemGathered);
	WeaponEnhancedHandle = APy_Grey::OnWeaponEnhanced.AddUObject(this, &UQuestComponent::HandleWeaponEnhanced);
}

void UQuestComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AMonster::OnMonsterKilled.Remove(MonsterKilledHandle);
	AGatherableObject::OnItemGathered.Remove(ItemGatheredHandle);
	APy_Grey::OnWeaponEnhanced.Remove(WeaponEnhancedHandle);

	Super::EndPlay(EndPlayReason);
}

void UQuestComponent::AcceptQuest(const FQuestData& InQuestData)
{
	ActiveQuest = InQuestData;
	ActiveQuest.CurrentCount = 0;
	ActiveQuest.bIsCompleted = false;
	bHasActiveQuest = true;

	if (!ActiveQuest.HasObjective())
	{
		ActiveQuest.bIsCompleted = true;
		GrantReward();
	}

	OnQuestTrackerChanged.Broadcast();
}

void UQuestComponent::HandleMonsterKilled(FName MonsterID, int32 ExpReward, int32 GoldReward)
{
	TryAdvanceObjective(EQuestObjectiveType::Kill, MonsterID, 1);
}

void UQuestComponent::HandleItemGathered(FName ItemID, int32 Count)
{
	TryAdvanceObjective(EQuestObjectiveType::Gather, ItemID, Count);
}

void UQuestComponent::HandleWeaponEnhanced(int32 NewLevel)
{
	TryAdvanceObjective(EQuestObjectiveType::Enhance, NAME_None, 1);
}

void UQuestComponent::TryAdvanceObjective(EQuestObjectiveType Type, FName InTargetID, int32 Amount)
{
	if (!bHasActiveQuest || ActiveQuest.bIsCompleted)
		return;

	if (ActiveQuest.ObjectiveType != Type)
		return;

	// Kill/Gather는 대상 ID까지 일치해야 함. Enhance는 대상 ID 없이 횟수만 센다.
	if (Type != EQuestObjectiveType::Enhance)
	{
		if (ActiveQuest.TargetID.IsNone() || ActiveQuest.TargetID != InTargetID)
			return;
	}

	ActiveQuest.CurrentCount = FMath::Min(ActiveQuest.CurrentCount + Amount, ActiveQuest.TargetCount);

	if (ActiveQuest.CurrentCount >= ActiveQuest.TargetCount)
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
