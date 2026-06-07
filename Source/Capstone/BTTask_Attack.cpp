#include "BTTask_Attack.h"

#include "MonsterAIController.h"
#include "Monster.h"
#include "MonsterAnim.h"

UBTTask_Attack::UBTTask_Attack()
{
	NodeName = TEXT("Attack");
	bNotifyTick = true;
	bAttack = true;
}

EBTNodeResult::Type UBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type eResult = Super::ExecuteTask(OwnerComp, NodeMemory);

	AMonsterAIController* pController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());

	if (!IsValid(pController))
		return EBTNodeResult::Failed;

	AMonster* pMonster = Cast<AMonster>(pController->GetPawn());

	if (!IsValid(pMonster))
		return EBTNodeResult::Failed;

	pMonster->AddAttackEndDelegate<UBTTask_Attack>(this, &UBTTask_Attack::AttackEnd);

	UMonsterAnim* pMonsterAnim = Cast<UMonsterAnim>(pMonster->GetMesh()->GetAnimInstance());

	if(!IsValid(pMonsterAnim))
		return EBTNodeResult::Failed;

	pMonsterAnim->ChangeAnim(EBaseAnim::Attack);
	pMonsterAnim->NormalAttack(true);

	bAttack = true;

	pMonster->SetAttack(true);


	return EBTNodeResult::InProgress;
}

void UBTTask_Attack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AMonsterAIController* pController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());

	AMonster* pMonster = Cast<AMonster>(pController->GetPawn());

	ACharacter* pTarget = pController->GetTarget();

	UMonsterAnim* pMonsterAnim = Cast<UMonsterAnim>(pMonster->GetMesh()->GetAnimInstance());

	if (!pTarget)
	{
		pController->StopMovement();

		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		pMonsterAnim->ChangeAnim(EBaseAnim::AttackDiscovery);
		return;
	}

	FVector vAILoc = pMonster->GetActorLocation();
	FVector vTargetLoc = pTarget->GetActorLocation();

	vAILoc.Z = 0.f;
	vTargetLoc.Z = 0.f;

	float fDist = FVector::Distance(vAILoc, vTargetLoc);

	if (fDist > pMonster->GetAttackDist())
	{
		if (!pMonster->GetAttack())
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);

			pMonsterAnim->ChangeAnim(EBaseAnim::AttackDiscovery);

			return;
		}
	}

	FVector vDir = pTarget->GetActorLocation() - pMonster->GetActorLocation();
	vDir.Normalize(); 

	pMonster->SetActorRotation(FRotator(0.f, vDir.Rotation().Yaw, 0.f));
}

void UBTTask_Attack::AttackEnd()
{
	bAttack = false;
}
