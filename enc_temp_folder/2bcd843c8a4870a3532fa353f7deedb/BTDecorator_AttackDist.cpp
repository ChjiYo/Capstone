#include "BTDecorator_AttackDist.h"
#include "MonsterAIController.h"
#include "Monster.h"

UBTDecorator_AttackDist::UBTDecorator_AttackDist()
{
	NodeName = TEXT("AttackDist");

	bCreateNodeInstance = true;
	bNotifyActivation = true;

	MinAttackDist = 0.f;
	MaxAttackDist = 200.f;
	CooldownTime = 3.f;

	LastExecutionTime = 0.f;
}

bool UBTDecorator_AttackDist::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	if (!Super::CalculateRawConditionValue(OwnerComp, NodeMemory))
		return false;

	AMonsterAIController* pController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());
	if (!IsValid(pController)) return false;

	AMonster* pMonster = Cast<AMonster>(pController->GetPawn());
	if (!IsValid(pMonster)) return false;

	ACharacter* pTarget = pController->GetTarget();
	if (!IsValid(pTarget)) return false;

	FVector vAILoc = pMonster->GetActorLocation();
	FVector vTargetLoc = pTarget->GetActorLocation();
	vAILoc.Z = 0.f;
	vTargetLoc.Z = 0.f;	

	float fDist = FVector::Distance(vAILoc, vTargetLoc);

	if (fDist >= MinAttackDist && fDist <= MaxAttackDist)
	{
		return true; 
	}

	return false; 
}

void UBTDecorator_AttackDist::OnNodeActivation(FBehaviorTreeSearchData& SearchData)
{
	Super::OnNodeActivation(SearchData);

	LastExecutionTime = GetWorld()->GetTimeSeconds();
}


