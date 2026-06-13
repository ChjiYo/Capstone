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
	ACharacter* pTarget = pController->GetTarget();
	AMonster* pMonster = Cast<AMonster>(pController->GetPawn()); // 몬스터 위치를 알기 위해 추가

	// 1. 타겟이나 몬스터가 유효하지 않으면 실패로 빠져나감
	if (!IsValid(pTarget) || !IsValid(pMonster))
	{
		pController->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 2. 몬스터와 타겟 사이의 거리를 계산 (Z축 무시)
	FVector vAILoc = pMonster->GetActorLocation();
	FVector vTargetLoc = pTarget->GetActorLocation();
	vAILoc.Z = 0.f;
	vTargetLoc.Z = 0.f;

	float fDist = FVector::Distance(vAILoc, vTargetLoc);

	// 3. 공격 가능한 거리(AcceptanceRadius) 안으로 들어왔다면 이동 종료!
	// (앞서 AttackDist에서 MaxAttackDist를 200.f로 하셨으니 동일하게 맞추는 것이 좋습니다)
	float AcceptanceRadius = 200.f;

	if (fDist <= AcceptanceRadius)
	{
		pController->StopMovement(); // 제자리에 멈춤
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded); // ⭐️ 성공 처리 후 태스크 탈출!
		return;
	}

	// 4. 아직 거리가 멀다면 계속해서 타겟을 향해 이동
	UAIBlueprintHelperLibrary::SimpleMoveToActor(pController, pTarget);

}
