#include "BTTask_TargetMove.h"
#include "MonsterAIController.h"
#include "Monster.h"
#include  "BehaviorTree/BlackboardComponent.h"

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

	// ⭐ 수정: 하드코딩된 평타 사거리 대신, 블랙보드에서 목표 사거리를 읽어옵니다.
	UBlackboardComponent* pBBComp = OwnerComp.GetBlackboardComponent();
	float fTargetStopDist = pMonster->GetAttackDist(); // 기본값 복사

	if (pBBComp)
	{
		fTargetStopDist = pBBComp->GetValueAsFloat(TargetMoveDistKey.SelectedKeyName);
	}

	// 블랙보드 사거리에 도달하면 정지
	if (fDist <= fTargetStopDist)
	{
		pController->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

}
