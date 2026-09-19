#include "BTTask_Dash.h"
#include "AIController.h"
#include "Monster.h"
#include "MonsterAnim.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"

UBTTask_Dash::UBTTask_Dash()
{
	NodeName = TEXT("Dash");
	bCreateNodeInstance = true;
	MyOwnerComp = nullptr;
}

EBTNodeResult::Type UBTTask_Dash::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!IsValid(AIController)) return EBTNodeResult::Failed;

	AActor* TargetActor = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(TargetKey.SelectedKeyName));
	if (!IsValid(TargetActor)) return EBTNodeResult::Failed;


	AMonster* pMonster = Cast<AMonster>(AIController->GetPawn());
	if (!IsValid(pMonster)) return EBTNodeResult::Failed;

	UMonsterAnim* pMonsterAnim = Cast<UMonsterAnim>(pMonster->GetMesh()->GetAnimInstance());
	if (!IsValid(pMonsterAnim)) return EBTNodeResult::Failed;

	FVector DashDirection = TargetActor->GetActorLocation() - pMonster->GetActorLocation();

	DashDirection.Z = 0.f;
	DashDirection.Normalize();

	pMonster->SetActorRotation(DashDirection.Rotation());

	float fDist = FVector::Distance(TargetActor->GetActorLocation(), pMonster->GetActorLocation());

	if (fDist <= 50.f)
		return EBTNodeResult::Succeeded;

	if (IsValid(DashMontage))
	{
		float PlayTime = pMonsterAnim->Montage_Play(DashMontage, 1.0f);

		if (PlayTime > 0.0f)
		{
			MyOwnerComp = &OwnerComp;

			FOnMontageEnded MontageEndedDelegate;
			MontageEndedDelegate.BindUObject(this, &UBTTask_Dash::OnDashMontageEnded);
			pMonsterAnim->Montage_SetEndDelegate(MontageEndedDelegate, DashMontage);

			return EBTNodeResult::InProgress;
		}
	}

	// 재생할 몽타주가 없으면 대시 없이 바로 완료 처리 (Task가 InProgress로 영원히 멈춰있지 않도록)
	return EBTNodeResult::Succeeded;
}

EBTNodeResult::Type UBTTask_Dash::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	MyOwnerComp = nullptr;
	return Super::AbortTask(OwnerComp, NodeMemory);
}

void UBTTask_Dash::OnDashMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (MyOwnerComp)
	{
		FinishLatentTask(*MyOwnerComp, EBTNodeResult::Succeeded);
		MyOwnerComp = nullptr;
	}
}