#include "ExperienceComponent.h"
#include "Monster.h"
#include "LevelTransitionSubsystem.h"

UExperienceComponent::UExperienceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	RecalculateRequiredExp();
}

void UExperienceComponent::BeginPlay()
{
	Super::BeginPlay();

	MonsterKilledHandle = AMonster::OnMonsterKilled.AddUObject(this, &UExperienceComponent::HandleMonsterKilled);

	// 포탈을 통해 방금 다른 레벨에서 넘어온 경우, 파괴되기 전 폰이 맡겨둔 레벨/경험치를 복원한다
	if (UGameInstance* GI = GetWorld()->GetGameInstance())
	{
		if (ULevelTransitionSubsystem* TransitionSubsystem = GI->GetSubsystem<ULevelTransitionSubsystem>())
		{
			int32 SavedLevel = 1;
			int32 SavedExp = 0;
			if (TransitionSubsystem->ConsumePendingExperience(SavedLevel, SavedExp))
			{
				RestoreExp(SavedLevel, SavedExp);
			}
		}
	}
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

void UExperienceComponent::RestoreExp(int32 InLevel, int32 InExp)
{
	CurrentLevel = FMath::Max(InLevel, 1);
	RecalculateRequiredExp();
	CurrentExp = InExp;

	OnExpChanged.Broadcast();
}

void UExperienceComponent::RecalculateRequiredExp()
{
	RequiredExp = FMath::RoundToInt(BaseRequiredExp * FMath::Pow(ExpGrowthRate, static_cast<float>(CurrentLevel - 1)));
}
