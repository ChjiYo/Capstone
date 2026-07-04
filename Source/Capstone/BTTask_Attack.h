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

	// Task를 안전하게 끝내기 위해 컴포넌트를 기억해둘 변수
	UBehaviorTreeComponent* MyOwnerComp;

private:
	bool bIsAttacking;
	
};
