#pragma once

#include "GameInfo.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CombatPatternSubsystem.generated.h"

// 몬스터 종류(MonsterID)별 공격 패턴 학습 상태
USTRUCT()
struct FPatternStats
{
	GENERATED_BODY()

	// 공격 패턴별 성공 추정치 (0~1). 크기는 그 몬스터의 AttackPatterns 개수를 따라감
	UPROPERTY()
	TArray<float> PatternWeights;

	UPROPERTY()
	int32 TotalAttacks = 0;
};

// 게임 세션 동안 몬스터가 플레이어의 행동 패턴을 관측·누적하는 GameInstance 수명 서브시스템.
// 신경망 없이 런타임 통계 + ε-greedy 밴딧으로 구현 (SaveGame 영속화는 범위 밖 — 세션 내 학습만 유지)
UCLASS()
class CAPSTONE_API UCombatPatternSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ---- 기록 ----

	// 몬스터의 공격 패턴이 명중했는지(bHit) 결과를 반영. AWeapon::WepaonBeginOverlap에서 TakeDamage 반환값으로 판정해 호출
	void RecordAttackResult(FName MonsterID, int32 PatternIndex, bool bHit);

	// APy_Grey::OnPlayerDodge 구독으로 자동 기록
	void RecordPlayerDodge();

	// 포션 사용 시점의 HP 비율 기록. Py_Grey::UsePotionItem에서 호출
	void RecordPlayerPotionUse(float HpRatio);

	// 몬스터-플레이어 교전 거리 샘플링. UBTService_Detect 틱에서 호출
	void RecordEngageDistance(float Dist);

	// ---- 조회 ----

	// ε-greedy로 패턴 인덱스를 고른다. NumPatterns가 0 이하이면 -1 반환(호출측이 폴백 처리)
	int32 SelectAttackPattern(FName MonsterID, int32 NumPatterns);

	float GetDodgeRate() const { return DodgeRate; }
	float GetAvgEngageDistance() const { return AvgEngageDistance; }

	// 아직 관측된 값이 없으면 false 반환(호출측이 조건 판단을 건너뛸 수 있도록)
	bool GetHealHpRatio(float& OutRatio) const;

	// 시연용 디버그 출력 (발표에서 학습 전/후 차이를 보여주기 위함)
	void PrintDebugStats(FName MonsterID) const;

protected:
	// EMA(지수이동평균) 갱신 계수. 최근 행동에 더 민감하게 반응
	UPROPERTY(EditAnywhere, Category = "Adaptive AI")
	float Alpha = 0.3f;

	// ε-greedy 탐험 확률
	UPROPERTY(EditAnywhere, Category = "Adaptive AI")
	float Epsilon = 0.2f;

	UPROPERTY()
	TMap<FName, FPatternStats> StatsByMonster;

	float DodgeRate = 0.f;
	float AvgEngageDistance = 0.f;

	float HealHpRatio = 0.f;
	bool bHasHealSample = false;

	FDelegateHandle PlayerDodgeHandle;
};
