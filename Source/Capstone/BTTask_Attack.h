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
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack Options", meta = (EditCondition = "PlayType == EAttackPlayType::UseMontage"))
	UAnimMontage* AttackMontage;

public:
	void AttackEnd();

private:
	bool bIsAttacking;
	
};
