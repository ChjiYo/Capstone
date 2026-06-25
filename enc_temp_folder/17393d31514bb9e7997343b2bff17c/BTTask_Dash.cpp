#include "BTTask_Dash.h"
#include "AIController.h"
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

	ACharacter* MonsterPawn = Cast<ACharacter>(AIController->GetPawn());
	if (!IsValid(MonsterPawn)) return EBTNodeResult::Failed;

	AActor* TargetActor = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(TargetKey.SelectedKeyName));
	if (!IsValid(TargetActor)) return EBTNodeResult::Failed;

	FVector DashDirection = TargetActor->GetActorLocation() - MonsterPawn->GetActorLocation();

	DashDirection.Z = 0.f;
	DashDirection.Normalize(); 

	MonsterPawn->SetActorRotation(DashDirection.Rotation());

	MonsterPawn->LaunchCharacter(DashDirection * DashPower, true, true);

	return EBTNodeResult::Succeeded;
}