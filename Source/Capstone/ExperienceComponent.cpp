#include "ExperienceComponent.h"
#include "Monster.h"

UExperienceComponent::UExperienceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	RecalculateRequiredExp();
}

void UExperienceComponent::BeginPlay()
{
	Super::BeginPlay();

	MonsterKilledHandle = AMonster::OnMonsterKilled.AddUObject(this, &UExperienceComponent::HandleMonsterKilled);
}

void UExperienceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AMonster::OnMonsterKilled.Remove(MonsterKilledHandle);

	Super::EndPlay(EndPlayReason);
}

void UExperienceComponent::HandleMonsterKilled(FName MonsterID, int32 ExpReward, int32 GoldReward)
{
	AddExp(ExpReward);
}

void UExperienceComponent::AddExp(int32 Amount)
{
	if (Amount <= 0)
		return;

	CurrentExp += Amount;

	while (RequiredExp > 0 && CurrentExp >= RequiredExp)
	{
		CurrentExp -= RequiredExp;
		++CurrentLevel;

		RecalculateRequiredExp();

		OnLevelUp.Broadcast(CurrentLevel);
	}

	OnExpChanged.Broadcast();
}

void UExperienceComponent::RecalculateRequiredExp()
{
	RequiredExp = FMath::RoundToInt(BaseRequiredExp * FMath::Pow(ExpGrowthRate, static_cast<float>(CurrentLevel - 1)));
}
