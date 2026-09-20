#include "CombatPatternSubsystem.h"
#include "Py_Grey.h"

void UCombatPatternSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PlayerDodgeHandle = APy_Grey::OnPlayerDodge.AddUObject(this, &UCombatPatternSubsystem::RecordPlayerDodge);
}

void UCombatPatternSubsystem::Deinitialize()
{
	APy_Grey::OnPlayerDodge.Remove(PlayerDodgeHandle);

	Super::Deinitialize();
}

void UCombatPatternSubsystem::RecordAttackResult(FName MonsterID, int32 PatternIndex, bool bHit)
{
	if (MonsterID.IsNone() || PatternIndex < 0)
		return;

	FPatternStats& Stats = StatsByMonster.FindOrAdd(MonsterID);

	if (Stats.PatternWeights.Num() <= PatternIndex)
	{
		Stats.PatternWeights.SetNumZeroed(PatternIndex + 1);

		for (int32 i = 0; i < Stats.PatternWeights.Num(); ++i)
		{
			if (Stats.PatternWeights[i] == 0.f)
				Stats.PatternWeights[i] = 0.5f;
		}
	}

	Stats.PatternWeights[PatternIndex] = (1.f - Alpha) * Stats.PatternWeights[PatternIndex] + Alpha * (bHit ? 1.f : 0.f);
	++Stats.TotalAttacks;
}

void UCombatPatternSubsystem::RecordPlayerDodge()
{
	DodgeRate = (1.f - Alpha) * DodgeRate + Alpha * 1.f;
}

void UCombatPatternSubsystem::RecordPlayerPotionUse(float HpRatio)
{
	HealHpRatio = bHasHealSample ? ((1.f - Alpha) * HealHpRatio + Alpha * HpRatio) : HpRatio;
	bHasHealSample = true;
}

void UCombatPatternSubsystem::RecordEngageDistance(float Dist)
{
	AvgEngageDistance = (AvgEngageDistance <= 0.f) ? Dist : (1.f - Alpha) * AvgEngageDistance + Alpha * Dist;
}

int32 UCombatPatternSubsystem::SelectAttackPattern(FName MonsterID, int32 NumPatterns)
{
	if (NumPatterns <= 0)
		return -1;

	FPatternStats& Stats = StatsByMonster.FindOrAdd(MonsterID);

	if (Stats.PatternWeights.Num() != NumPatterns)
	{
		Stats.PatternWeights.Init(0.5f, NumPatterns);
	}

	if (FMath::FRand() <= Epsilon)
	{
		return FMath::RandRange(0, NumPatterns - 1);
	}

	int32 BestIndex = 0;
	float BestWeight = Stats.PatternWeights[0];

	for (int32 i = 1; i < Stats.PatternWeights.Num(); ++i)
	{
		if (Stats.PatternWeights[i] > BestWeight)
		{
			BestWeight = Stats.PatternWeights[i];
			BestIndex = i;
		}
	}

	return BestIndex;
}

float UCombatPatternSubsystem::GetPatternWeight(FName MonsterID, int32 PatternIndex) const
{
	if (const FPatternStats* Stats = StatsByMonster.Find(MonsterID))
	{
		if (Stats->PatternWeights.IsValidIndex(PatternIndex))
			return Stats->PatternWeights[PatternIndex];
	}

	return 0.5f;
}

bool UCombatPatternSubsystem::GetHealHpRatio(float& OutRatio) const
{
	OutRatio = HealHpRatio;
	return bHasHealSample;
}

void UCombatPatternSubsystem::PrintDebugStats(FName MonsterID) const
{
	FString WeightsStr;

	if (const FPatternStats* Stats = StatsByMonster.Find(MonsterID))
	{
		for (int32 i = 0; i < Stats->PatternWeights.Num(); ++i)
		{
			WeightsStr += FString::Printf(TEXT("[%d]=%.2f "), i, Stats->PatternWeights[i]);
		}
	}

	PrintViewport(1.f, FColor::Cyan, FString::Printf(TEXT("%s DodgeRate=%.2f AvgDist=%.0f Weights: %s"),
		*MonsterID.ToString(), DodgeRate, AvgEngageDistance, *WeightsStr));
}
