#include "BTTask_BossApproach.h"
#include "MonsterAIController.h"
#include "Mon_Narbash.h"

UBTTask_BossApproach::UBTTask_BossApproach()
{
	NodeName = TEXT("Boss Approach");
	bNotifyTick = true;
	bCreateNodeInstance = true;
}

FString UBTTask_BossApproach::GetStaticDescription() const
{
	return TEXT("다음 스킬의 사거리까지 접근 (후딜/스턴 중에는 제자리에서 대기)");
}

EBTNodeResult::Type UBTTask_BossApproach::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	MoveRefreshTimer = 0.f;

	AMonsterAIController* pController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());
	if (!IsValid(pController) || !IsValid(pController->GetTarget()))
		return EBTNodeResult::Failed;

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_BossApproach::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AMonsterAIController* pController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner()))
		pController->StopMovement();

	return EBTNodeResult::Aborted;
}

void UBTTask_BossApproach::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AMonsterAIController* pController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());
	if (!IsValid(pController)) return;

	AMon_Narbash* pBoss = Cast<AMon_Narbash>(pController->GetPawn());
	ACharacter* pTarget = pController->GetTarget();

	if (!IsValid(pBoss) || !IsValid(pTarget))
	{
		pController->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 스턴/스킬 중에는 이동 명령을 내리지 않는다
	if (pBoss->IsBusy())
	{
		pController->StopMovement();
		return;
	}

	if (pBoss->IsPendingSkillReady())
	{
		pController->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// 후딜: 제자리에서 플레이어를 노려본다 (뒤돌아서 등을 보이지 않도록 회전만 갱신)
	if (pBoss->IsRecovering())
	{
		pController->StopMovement();
		pBoss->FaceTarget(DeltaSeconds, pBoss->RecoveryTurnRate);
		return;
	}

	const float Dist = pBoss->GetDistanceToTarget2D();
	const float DesiredDist = pBoss->GetPendingApproachDistance();

	if (Dist <= DesiredDist)
	{
		// 이미 원하는 거리 안이지만 스킬 쿨다운 등으로 아직 못 쓰는 상태: 제자리에서 견제하며 대기
		pController->StopMovement();
		pBoss->FaceTarget(DeltaSeconds, pBoss->RecoveryTurnRate);
		return;
	}

	MoveRefreshTimer -= DeltaSeconds;

	if (MoveRefreshTimer <= 0.f)
	{
		MoveRefreshTimer = 0.25f;

		// 캡슐 반지름 등을 감안해 조금 안쪽까지 접근하도록 허용 반경을 줄여 잡는다
		const float AcceptRadius = FMath::Max(DesiredDist - 100.f, 30.f);
		pController->MoveToActor(pTarget, AcceptRadius, true, true, false, nullptr, true);
	}
}
