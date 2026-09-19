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
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack Options", meta = (EditCondition = "PlayType == EAttackPlayType::UseMontage"))
	UAnimMontage* DashMontage;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector TargetKey;

private:
	UFUNCTION()
	void OnDashMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// Task를 안전하게 끝내기 위해 컴포넌트를 기억해둘 변수 (인스턴스 상태이므로 bCreateNodeInstance 필요)
	UBehaviorTreeComponent* MyOwnerComp;
};
