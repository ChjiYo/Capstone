#include "BTTask_Dash.h"
#include "AIController.h"
#include "Monster.h"
#include "MonsterAnim.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"

UBTTask_Dash::UBTTask_Dash()
{
	NodeName = TEXT("Dash");
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

	

	if (IsValid(DashMontage))
	{
		pMonsterAnim->Montage_Play(DashMontage, 1.0f);
	}

	FVector DashDirection = TargetActor->GetActorLocation() - pMonster->GetActorLocation();

	DashDirection.Z = 0.f;
	DashDirection.Normalize(); 

	pMonster->SetActorRotation(DashDirection.Rotation());

	pMonster->LaunchCharacter(DashDirection * DashPower, true, true);

	float fDist = FVector::Distance(TargetActor->GetActorLocation(), pMonster->GetActorLocation());

	if (fDist <= 50.f)
		return EBTNodeResult::Succeeded;

	return  EBTNodeResult::InProgress;
}