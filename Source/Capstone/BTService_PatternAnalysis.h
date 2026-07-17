#pragma once

#include "GameInfo.h"
#include "BehaviorTree/BTService.h"
#include "BTService_PatternAnalysis.generated.h"

// 7단계 적응형 AI: 플레이어 성향(회피율/교전 거리/회복 습관)을 조회해 이번 공격 패턴과
// 페인트/공격성 변조 값을 블랙보드에 기록한다. 값 자체의 계산은 UCombatPatternSubsystem이 담당하고,
// 이 서비스는 "몬스터 행동에 어떻게 반영할지" 임계값/배율만 들고 있다
UCLASS()
class CAPSTONE_API UBTService_PatternAnalysis : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_PatternAnalysis();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

public:
	// 회피율이 이 값을 넘으면 FeintChance를 올리기 시작
	UPROPERTY(EditAnywhere, Category = "Adaptive AI")
	float DodgeRateThreshold = 0.5f;

	// 회피율 100%일 때 도달하는 최대 페인트 확률
	UPROPERTY(EditAnywhere, Category = "Adaptive AI")
	float MaxFeintChance = 0.6f;

	// 이 거리 이상 벌어지면 원거리 유지 성향으로 보고 Aggression을 올림
	UPROPERTY(EditAnywhere, Category = "Adaptive AI")
	float LongRangeThreshold = 600.f;

	UPROPERTY(EditAnywhere, Category = "Adaptive AI")
	float MaxAggression = 1.f;

	// 플레이어의 평소 회복 HP 비율 근처(±Tolerance)일 때 Aggression을 올림
	UPROPERTY(EditAnywhere, Category = "Adaptive AI")
	float HealRatioTolerance = 0.1f;

	// 시연/디버그용 화면 출력 여부
	UPROPERTY(EditAnywhere, Category = "Adaptive AI")
	bool bShowDebugStats = false;
};
