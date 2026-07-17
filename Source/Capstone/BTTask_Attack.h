#pragma once

#include "GameInfo.h"
#include "CharBaseAnim.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Attack.generated.h"

UCLASS()
class CAPSTONE_API UBTTask_Attack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_Attack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory);
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack Options", meta = (EditCondition = "PlayType == EAttackPlayType::UseMontage"))
	UAnimMontage* AttackMontage;

public:
	void AttackEnd();

private:
	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// AttackMontage 재생을 실제로 시작하고 InProgress를 반환하는 공통 처리 (즉시 실행 / 페인트 지연 후 실행 둘 다 여기로 모인다)
	EBTNodeResult::Type StartAttackMontage(UBehaviorTreeComponent& OwnerComp, class AMonster* pMonster, class UMonsterAnim* pMonsterAnim);

	// Task를 안전하게 끝내기 위해 컴포넌트를 기억해둘 변수
	UBehaviorTreeComponent* MyOwnerComp;

private:
	bool bIsAttacking;

	// 페인트(속임수) 대기 중인지. true인 동안은 TickTask에서 남은 시간만 줄이고 몽타주를 재생하지 않는다
	bool bIsFeinting;
	float FeintTimeRemaining;

};
