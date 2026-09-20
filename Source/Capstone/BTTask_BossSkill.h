#pragma once

#include "GameInfo.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_BossSkill.generated.h"

// 나르바쉬 보스가 미리 골라 둔 스킬(PendingSkill)을 실제로 시전하고, 스킬이 끝날 때까지 InProgress로 대기한다.
// 어떤 스킬을 쓸지/언제 쓸지 판단은 AMon_Narbash의 브레인이 담당하고, 이 노드는 실행만 맡는다
UCLASS()
class CAPSTONE_API UBTTask_BossSkill : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_BossSkill();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

private:
	void OnSkillFinished(bool bCompleted);
	void Unbind();

	TWeakObjectPtr<class AMon_Narbash> Boss;
	UBehaviorTreeComponent* MyOwnerComp = nullptr;
	FDelegateHandle FinishedHandle;
};
