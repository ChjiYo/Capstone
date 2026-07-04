#include "BTTask_Attack.h"

#include "MonsterAIController.h"
#include "Monster.h"
#include "MonsterAnim.h"

UBTTask_Attack::UBTTask_Attack()
{
	NodeName = TEXT("Attack");
	bNotifyTick = true;
	bCreateNodeInstance = true;
	bIsAttacking = false;
}

EBTNodeResult::Type UBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type eResult = Super::ExecuteTask(OwnerComp, NodeMemory);

	AMonsterAIController* pController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());
	if (!IsValid(pController)) return EBTNodeResult::Failed;

	AMonster* pMonster = Cast<AMonster>(pController->GetPawn());
	if (!IsValid(pMonster)) return EBTNodeResult::Failed;

	UMonsterAnim* pMonsterAnim = Cast<UMonsterAnim>(pMonster->GetMesh()->GetAnimInstance());
	if (!IsValid(pMonsterAnim)) return EBTNodeResult::Failed;

	pMonster->AddAttackEndDelegate<UBTTask_Attack>(this, &UBTTask_Attack::AttackEnd);
		
	if (IsValid(AttackMontage))
	{
		pMonsterAnim->Montage_Play(AttackMontage, 1.0f);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Montage is missing on BT Node!"));
			return EBTNodeResult::Failed;
	}
	
	bIsAttacking = true;

	return EBTNodeResult::InProgress;
}

void UBTTask_Attack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	if (!bIsAttacking)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	AMonsterAIController* pController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());
	if (!IsValid(pController)) return;

	ACharacter* pTarget = pController->GetTarget();
	AMonster* pMonster = Cast<AMonster>(pController->GetPawn());

	if (!IsValid(pTarget))
	{
		pController->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FVector vDir = pTarget->GetActorLocation() - pMonster->GetActorLocation();
	vDir.Z = 0.f;
	vDir.Normalize();
	pMonster->SetActorRotation(FRotator(0.f, vDir.Rotation().Yaw, 0.f));
}

void UBTTask_Attack::AttackEnd()
{
	bIsAttacking = false;
}
