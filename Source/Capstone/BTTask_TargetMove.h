#pragma once

#include "GameInfo.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_TargetMove.generated.h"

UCLASS()
class CAPSTONE_API UBTTask_TargetMove : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_TargetMove();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory);
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds);

private:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetMoveDistKey;
};
