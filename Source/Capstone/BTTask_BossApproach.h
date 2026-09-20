#pragma once

#include "GameInfo.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_BossApproach.generated.h"

// 다음 스킬을 쓸 수 있는 거리까지 플레이어에게 다가가는 태스크. 스킬 직후의 후딜(경직) 동안에는
// 이동하지 않고 플레이어를 노려보며 서 있고, 스턴 중에는 아무것도 하지 않는다.
// 스킬을 쓸 수 있게 되면(UBTDecorator_BossSkillReady가 참이 되면) 상위 우선순위 브랜치가 이 태스크를 끊는다
UCLASS()
class CAPSTONE_API UBTTask_BossApproach : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_BossApproach();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;

private:
	// 이동 명령을 매 틱 다시 내리면 경로 탐색 비용이 커지므로 이 간격으로만 갱신한다
	float MoveRefreshTimer = 0.f;
};
