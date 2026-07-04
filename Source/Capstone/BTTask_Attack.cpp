#include "BTTask_Attack.h"

#include "MonsterAIController.h"
#include "Monster.h"
#include "MonsterAnim.h"

UBTTask_Attack::UBTTask_Attack()
{
	NodeName = TEXT("Attack");
	bNotifyTick = true;
	bCreateNodeInstance = true; 
	MyOwnerComp = nullptr;
}

EBTNodeResult::Type UBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);

	AMonsterAIController* pController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());
	if (!IsValid(pController)) return EBTNodeResult::Failed;

	AMonster* pMonster = Cast<AMonster>(pController->GetPawn());
	if (!IsValid(pMonster)) return EBTNodeResult::Failed;

	UMonsterAnim* pMonsterAnim = Cast<UMonsterAnim>(pMonster->GetMesh()->GetAnimInstance());
	if (!IsValid(pMonsterAnim)) return EBTNodeResult::Failed;

	if (IsValid(AttackMontage))
	{
		float PlayTime = pMonsterAnim->Montage_Play(AttackMontage, 1.0f);

		if (PlayTime > 0.0f)
		{
			MyOwnerComp = &OwnerComp;

			FOnMontageEnded MontageEndedDelegate;
			MontageEndedDelegate.BindUObject(this, &UBTTask_Attack::OnAttackMontageEnded);
			pMonsterAnim->Montage_SetEndDelegate(MontageEndedDelegate, AttackMontage);

			return EBTNodeResult::InProgress;
		}
	}

	return EBTNodeResult::Failed;
}

EBTNodeResult::Type UBTTask_Attack::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	MyOwnerComp = nullptr;
	return Super::AbortTask(OwnerComp, NodeMemory);
}

void UBTTask_Attack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AMonsterAIController* pController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());
	if (!IsValid(pController)) return;

	AMonster* pMonster = Cast<AMonster>(pController->GetPawn());
	ACharacter* pTarget = pController->GetTarget();

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

void UBTTask_Attack::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (MyOwnerComp)
	{
		// 정상 종료(Succeeded) 처리를 하여 다음 행동으로 넘어가게 합니다.
		FinishLatentTask(*MyOwnerComp, EBTNodeResult::Succeeded);
		MyOwnerComp = nullptr;
	}
}

void UBTTask_Attack::AttackEnd()
{
	bIsAttacking = false;
}
