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

	PlayType = EAttackPlayType::UseAnimState;
	AttackAnimState = EBaseAnim::Attack;
}

EBTNodeResult::Type UBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type eResult = Super::ExecuteTask(OwnerComp, NodeMemory);

	// 컨트롤러, 폰, 애님 인스턴스 가져오기 (이전과 동일)
	AMonsterAIController* pController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());
	if (!IsValid(pController)) return EBTNodeResult::Failed;

	AMonster* pMonster = Cast<AMonster>(pController->GetPawn());
	if (!IsValid(pMonster)) return EBTNodeResult::Failed;

	UMonsterAnim* pMonsterAnim = Cast<UMonsterAnim>(pMonster->GetMesh()->GetAnimInstance());
	if (!IsValid(pMonsterAnim)) return EBTNodeResult::Failed;

	// 공격 종료 델리게이트 바인딩
	pMonster->AddAttackEndDelegate<UBTTask_Attack>(this, &UBTTask_Attack::AttackEnd);

	// ⭐️ 여기서 공격 방식(PlayType)에 따라 로직을 나눕니다 ⭐️
	if (PlayType == EAttackPlayType::UseAnimState)
	{
		// 1. 애님 스테이트 머신 방식 (기본 공격)
		pMonsterAnim->ChangeAnim(AttackAnimState);
		pMonsterAnim->NormalAttack(true);
	}
	else if (PlayType == EAttackPlayType::UseMontage)
	{
		// 2. 애님 몽타주 방식 (스킬)
		if (IsValid(AttackMontage))
		{
			pMonsterAnim->Montage_Play(AttackMontage, 1.0f);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Montage is missing on BT Node!"));
			return EBTNodeResult::Failed;
		}
	}

	pMonster->SetAttack(true);
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
