#include "BTTask_TargetMove.h"
#include "MonsterAIController.h"
#include "Monster.h"

UBTTask_TargetMove::UBTTask_TargetMove()
{
	NodeName = TEXT("TargetMove");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_TargetMove::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type  eResult = Super::ExecuteTask(OwnerComp, NodeMemory);

	AMonsterAIController* pController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());

	if (!IsValid(pController))
		return EBTNodeResult::Failed;

	AMonster* pMonster = Cast<AMonster>(pController->GetPawn());

	if (!IsValid(pMonster))
		return EBTNodeResult::Failed;

	ACharacter* pTarget = pController->GetTarget();

	if (!IsValid(pTarget))
		return EBTNodeResult::Failed;

	return EBTNodeResult::InProgress;
}

void UBTTask_TargetMove::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AMonsterAIController* pController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());

	AMonster* pMonster = Cast<AMonster>(pController->GetPawn());

	ACharacter* pTarget = pController->GetTarget();

	if (!pTarget)
	{
		pController->StopMovement();

		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	UAIBlueprintHelperLibrary::SimpleMoveToActor(pController, pTarget);

	FVector vAILoc = pMonster->GetActorLocation();
	FVector vTargetLoc = pTarget->GetActorLocation();

	vAILoc.Z = 0.f;
	vTargetLoc.Z = 0.f;

	float fDist = FVector::Distance(vAILoc, vTargetLoc);

	if (fDist <= pMonster->GetAttackDist())
	{
		pController->StopMovement();

		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

}
