#include "MonsterAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Monster.h"

AMonsterAIController::AMonsterAIController()
{
	static ConstructorHelpers::FObjectFinder <UBehaviorTree> BTAsset (TEXT("BehaviorTree'/Game/UE_Asset/Character/Monster/BT_Monster.BT_Monster'"));
	
	if (BTAsset.Succeeded())
		m_pBTAsset = BTAsset.Object;

	static ConstructorHelpers::FObjectFinder <UBlackboardData> BBAsset(TEXT("BlackboardData'/Game/UE_Asset/Character/Monster/BB_Monster.BB_Monster'"));

	if (BTAsset.Succeeded())
		m_pBBAsset = BBAsset.Object;

}

void AMonsterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	UBlackboardComponent* BBCom = nullptr;
	if (UseBlackboard(m_pBBAsset, BBCom))
	{
		if (!RunBehaviorTree(m_pBTAsset))
		{

		}
	}
}

void AMonsterAIController::OnUnPossess()
{
	Super::OnUnPossess();
}

void AMonsterAIController::SetTarget(UObject* pTarget)
{
	Blackboard->SetValueAsObject(TEXT("Target"), pTarget);
}

ACharacter* AMonsterAIController::GetTarget() const
{
	return Cast<ACharacter>(Blackboard->GetValueAsObject(TEXT("Target")));
}
