#include "BTTask_Attack.h"

#include "MonsterAIController.h"
#include "Monster.h"
#include "MonsterAnim.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_Attack::UBTTask_Attack()
{
	NodeName = TEXT("Attack");
	bNotifyTick = true;
	bCreateNodeInstance = true;
	MyOwnerComp = nullptr;
	bIsFeinting = false;
	FeintTimeRemaining = 0.f;
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

	// 이번 공격에 쓸 패턴 인덱스는 UBTService_PatternAnalysis가 미리 블랙보드에 기록해 둔다
	if (pMonster->AttackPatterns.Num() > 0)
	{
		int32 SelectedPattern = OwnerComp.GetBlackboardComponent()->GetValueAsInt(TEXT("SelectedPattern"));
		pMonster->CurrentPatternIndex = FMath::Clamp(SelectedPattern, 0, pMonster->AttackPatterns.Num() - 1);
	}
	else
	{
		pMonster->CurrentPatternIndex = 0;
	}

	// 회피 위주 플레이어에게는 확률적으로 공격 시작을 늦춰 회피 타이밍을 흐트러뜨린다 (7단계 적응형 AI)
	float FeintChance = OwnerComp.GetBlackboardComponent()->GetValueAsFloat(TEXT("FeintChance"));

	if (FeintChance > 0.f && FMath::FRand() <= FeintChance)
	{
		bIsFeinting = true;
		FeintTimeRemaining = FMath::RandRange(0.3f, 0.6f);
		MyOwnerComp = &OwnerComp;
		return EBTNodeResult::InProgress;
	}

	return StartAttackMontage(OwnerComp, pMonster, pMonsterAnim);
}

EBTNodeResult::Type UBTTask_Attack::StartAttackMontage(UBehaviorTreeComponent& OwnerComp, AMonster* pMonster, UMonsterAnim* pMonsterAnim)
{
	UAnimMontage* MontageToPlay = (pMonster->AttackPatterns.Num() > 0) ? pMonster->AttackPatterns[pMonster->CurrentPatternIndex] : AttackMontage;

	if (!IsValid(MontageToPlay))
		MontageToPlay = AttackMontage;

	if (IsValid(MontageToPlay))
	{
		float PlayTime = pMonsterAnim->Montage_Play(MontageToPlay, 1.0f);

		if (PlayTime > 0.0f)
		{
			MyOwnerComp = &OwnerComp;

			FOnMontageEnded MontageEndedDelegate;
			MontageEndedDelegate.BindUObject(this, &UBTTask_Attack::OnAttackMontageEnded);
			pMonsterAnim->Montage_SetEndDelegate(MontageEndedDelegate, MontageToPlay);

			return EBTNodeResult::InProgress;
		}
	}

	return EBTNodeResult::Failed;
}

EBTNodeResult::Type UBTTask_Attack::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	MyOwnerComp = nullptr;
	bIsFeinting = false;
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
		bIsFeinting = false;
		pController->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FVector vDir = pTarget->GetActorLocation() - pMonster->GetActorLocation();
	vDir.Z = 0.f;
	vDir.Normalize();
	pMonster->SetActorRotation(FRotator(0.f, vDir.Rotation().Yaw, 0.f));

	if (bIsFeinting)
	{
		FeintTimeRemaining -= DeltaSeconds;

		if (FeintTimeRemaining <= 0.f)
		{
			bIsFeinting = false;

			UMonsterAnim* pMonsterAnim = Cast<UMonsterAnim>(pMonster->GetMesh()->GetAnimInstance());

			if (!IsValid(pMonsterAnim) || StartAttackMontage(OwnerComp, pMonster, pMonsterAnim) == EBTNodeResult::Failed)
			{
				FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			}
		}
	}
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
