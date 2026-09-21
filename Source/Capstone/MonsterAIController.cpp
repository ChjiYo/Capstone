#include "MonsterAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameplayTagContainer.h"
#include "Monster.h"

AMonsterAIController::AMonsterAIController()
{
	m_pBTAsset = nullptr;

    static ConstructorHelpers::FObjectFinder <UBehaviorTree> BTAsset(TEXT("BehaviorTree'/Game/UE_Asset/Character/Monster/BT_Monster.BT_Monster'"));
    if (BTAsset.Succeeded())
    {
        m_pBTAsset = BTAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder <UBlackboardData> BBAsset(TEXT("BlackboardData'/Game/UE_Asset/Character/Monster/BB_Monster.BB_Monster'"));
    if (BBAsset.Succeeded())
    {
        m_pBBAsset = BBAsset.Object;
    }

}

void AMonsterAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    AMonster* pMonster = Cast<AMonster>(InPawn);
    if (IsValid(pMonster))
    {
        UBlackboardComponent* BBCom = nullptr;
        if (UseBlackboard(m_pBBAsset, BBCom))
        {
            if (RunBehaviorTree(m_pBTAsset))
            {
                UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComponent);

                UBehaviorTree* CombatTree = pMonster->GetCombatBehaviorTree(m_pBBAsset);

                if (IsValid(BTComp) && IsValid(CombatTree))
                {
                    FGameplayTag InjectionTag = FGameplayTag::RequestGameplayTag(TEXT("AI.Subtree.Combat"));
                    BTComp->SetDynamicSubtree(InjectionTag, CombatTree);
                }
            }
        }
    }
}

void AMonsterAIController::OnUnPossess()
{
	Super::OnUnPossess();
}

void AMonsterAIController::SetTarget(UObject* pTarget)
{
	// UBTService_Detect 가 매 틱 호출하므로 "타겟 없음 -> 있음"으로 바뀌는 순간에만 경계음을 낸다
	const bool bHadTarget = Blackboard->GetValueAsObject(TEXT("Target")) != nullptr;

	Blackboard->SetValueAsObject(TEXT("Target"), pTarget);

	if (!bHadTarget && pTarget)
	{
		if (AMonster* Monster = Cast<AMonster>(GetPawn()))
			Monster->PlayAlertSFX();
	}
}

ACharacter* AMonsterAIController::GetTarget() const
{
	return Cast<ACharacter>(Blackboard->GetValueAsObject(TEXT("Target")));
}
