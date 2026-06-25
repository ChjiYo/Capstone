#pragma once

#include "GameInfo.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Dash.generated.h"
UCLASS()
class CAPSTONE_API UBTTask_Dash : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_Dash();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack Options", meta = (EditCondition = "PlayType == EAttackPlayType::UseMontage"))
	UAnimMontage* DashMontage;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector TargetKey;

	UPROPERTY(EditAnywhere, Category = "Dash")
	float DashPower = 2000.f;


	
};
