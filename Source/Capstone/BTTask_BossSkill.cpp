#include "BTTask_BossSkill.h"
#include "MonsterAIController.h"
#include "Mon_Narbash.h"

UBTTask_BossSkill::UBTTask_BossSkill()
{
	NodeName = TEXT("Boss Skill");
	bCreateNodeInstance = true;
}

FString UBTTask_BossSkill::GetStaticDescription() const
{
	return TEXT("나르바쉬가 선택해 둔 스킬을 시전하고 끝날 때까지 대기");
}

EBTNodeResult::Type UBTTask_BossSkill::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AMonsterAIController* pController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());
	if (!IsValid(pController)) return EBTNodeResult::Failed;

	AMon_Narbash* pBoss = Cast<AMon_Narbash>(pController->GetPawn());
	if (!IsValid(pBoss)) return EBTNodeResult::Failed;

	if (!pBoss->BeginPendingSkill())
		return EBTNodeResult::Failed;

	Boss = pBoss;
	MyOwnerComp = &OwnerComp;
	FinishedHandle = pBoss->OnSkillFinished.AddUObject(this, &UBTTask_BossSkill::OnSkillFinished);

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_BossSkill::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 구독을 먼저 끊어야 CancelSkill이 방송하는 종료 이벤트가 이미 끝난 태스크를 다시 FinishLatentTask하지 않는다
	AMon_Narbash* pBoss = Boss.Get();
	Unbind();

	if (IsValid(pBoss))
		pBoss->CancelSkill();

	return EBTNodeResult::Aborted;
}

void UBTTask_BossSkill::OnSkillFinished(bool bCompleted)
{
	UBehaviorTreeComponent* OwnerComp = MyOwnerComp;
	Unbind();

	if (OwnerComp)
		FinishLatentTask(*OwnerComp, bCompleted ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
}

void UBTTask_BossSkill::Unbind()
{
	if (AMon_Narbash* pBoss = Boss.Get())
		pBoss->OnSkillFinished.Remove(FinishedHandle);

	FinishedHandle.Reset();
	MyOwnerComp = nullptr;
}
