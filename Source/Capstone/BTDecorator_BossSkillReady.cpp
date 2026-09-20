#include "BTDecorator_BossSkillReady.h"
#include "MonsterAIController.h"
#include "Mon_Narbash.h"

UBTDecorator_BossSkillReady::UBTDecorator_BossSkillReady()
{
	NodeName = TEXT("Boss Skill Ready");

	// 이 조건이 참이 되는 순간 낮은 우선순위(접근 이동) 브랜치를 끊는다
	FlowAbortMode = EBTFlowAbortMode::LowerPriority;

	INIT_DECORATOR_NODE_NOTIFY_FLAGS();
}

FString UBTDecorator_BossSkillReady::GetStaticDescription() const
{
	return TEXT("선택된 스킬을 지금 쓸 수 있음 (사거리/쿨다운/후딜)");
}

bool UBTDecorator_BossSkillReady::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AMonsterAIController* pController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());
	if (!IsValid(pController)) return false;

	AMon_Narbash* pBoss = Cast<AMon_Narbash>(pController->GetPawn());
	if (!IsValid(pBoss)) return false;

	return pBoss->IsPendingSkillReady();
}

void UBTDecorator_BossSkillReady::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	ConditionalFlowAbort(OwnerComp, EBTDecoratorAbortRequest::ConditionResultChanged);
}
