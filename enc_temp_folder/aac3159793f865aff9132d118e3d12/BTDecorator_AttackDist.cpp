#include "BTDecorator_AttackDist.h"
#include "MonsterAIController.h"
#include "Monster.h"

UBTDecorator_AttackDist::UBTDecorator_AttackDist()
{
	NodeName = TEXT("AttackDist");
}

bool UBTDecorator_AttackDist::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	bool bResult = Super::CalculateRawConditionValue(OwnerComp, NodeMemory);

	AMonsterAIController* pController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());

	if (!IsValid(pController))
		return false;

	AMonster* pMonster = Cast<AMonster>(pController->GetPawn());

	if (!IsValid(pMonster))
		return false;

	//if (pMonster->GetAttack())
	//{
	//	pController->StopMovement();
	//	return true;
	//}

	ACharacter* pTarget = pController->GetTarget();

	if (!IsValid(pTarget))
		return false;

	FVector vAILoc = pMonster->GetActorLocation();
	FVector vTargetLoc = pTarget->GetActorLocation();

	vAILoc.Z = 0.f;
	vTargetLoc.Z = 0.f;

	float fDist = FVector::Distance(vAILoc, vTargetLoc);

	if (fDist <= pMonster->GetAttackDist())
	{
		pController->StopMovement();
		return true;
	}
	
	return false;
}


