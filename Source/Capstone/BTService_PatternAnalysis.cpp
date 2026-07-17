#include "BTService_PatternAnalysis.h"
#include "MonsterAIController.h"
#include "Monster.h"
#include "CharBase.h"
#include "CombatPatternSubsystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"

UBTService_PatternAnalysis::UBTService_PatternAnalysis()
{
	NodeName = "Pattern Analysis";
	Interval = 0.5f;
	RandomDeviation = 0.f;
}

void UBTService_PatternAnalysis::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AMonsterAIController* pController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());
	if (!IsValid(pController)) return;

	AMonster* pMonster = Cast<AMonster>(pController->GetPawn());
	if (!IsValid(pMonster)) return;

	UCombatPatternSubsystem* Subsystem = UGameplayStatics::GetGameInstance(pMonster)->GetSubsystem<UCombatPatternSubsystem>();
	if (!Subsystem) return;

	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (!IsValid(BBComp)) return;

	// 1. 이번 공격에 쓸 패턴 선택 (ε-greedy)
	int32 SelectedPattern = Subsystem->SelectAttackPattern(pMonster->MonsterID, pMonster->AttackPatterns.Num());
	BBComp->SetValueAsInt(TEXT("SelectedPattern"), FMath::Max(SelectedPattern, 0));

	// 2. 회피율 -> 페인트 확률
	float DodgeRate = Subsystem->GetDodgeRate();
	float FeintChance = 0.f;

	if (DodgeRate > DodgeRateThreshold)
	{
		float ExcessRatio = (DodgeRate - DodgeRateThreshold) / FMath::Max(1.f - DodgeRateThreshold, KINDA_SMALL_NUMBER);
		FeintChance = FMath::Clamp(ExcessRatio, 0.f, 1.f) * MaxFeintChance;
	}

	BBComp->SetValueAsFloat(TEXT("FeintChance"), FeintChance);

	// 3. 원거리 유지 성향 + 회복 타이밍 -> 공격성
	float Aggression = 0.f;
	float AvgDist = Subsystem->GetAvgEngageDistance();

	if (AvgDist > LongRangeThreshold)
	{
		float ExcessRatio = (AvgDist - LongRangeThreshold) / LongRangeThreshold;
		Aggression = FMath::Clamp(ExcessRatio, 0.f, 1.f) * MaxAggression;
	}

	float HealHpRatio = 0.f;

	if (Subsystem->GetHealHpRatio(HealHpRatio))
	{
		ACharacter* pTarget = pController->GetTarget();
		ACharBase* pTargetChar = Cast<ACharBase>(pTarget);

		if (IsValid(pTargetChar) && FMath::Abs(pTargetChar->GetHpRatio() - HealHpRatio) <= HealRatioTolerance)
		{
			Aggression = FMath::Max(Aggression, MaxAggression);
		}
	}

	BBComp->SetValueAsFloat(TEXT("Aggression"), Aggression);

	if (bShowDebugStats)
	{
		Subsystem->PrintDebugStats(pMonster->MonsterID);
	}
}
