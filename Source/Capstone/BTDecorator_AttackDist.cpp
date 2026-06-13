#include "BTDecorator_AttackDist.h"
#include "MonsterAIController.h"
#include "Monster.h"

UBTDecorator_AttackDist::UBTDecorator_AttackDist()
{
	NodeName = TEXT("AttackDist");
	// 쿨타임, 활성화 체크 등 불필요한 것 삭제
	MinAttackDist = 0.f;
	MaxAttackDist = 200.f;
}

bool UBTDecorator_AttackDist::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	if (!Super::CalculateRawConditionValue(OwnerComp, NodeMemory)) return false;

	AMonsterAIController* pController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());
	if (!IsValid(pController)) return false;

	AMonster* pMonster = Cast<AMonster>(pController->GetPawn());
	if (!IsValid(pMonster)) return false;

	ACharacter* pTarget = pController->GetTarget();
	if (!IsValid(pTarget)) return false;

	// 거리 계산 (Z축 무시)
	FVector vAILoc = pMonster->GetActorLocation();
	FVector vTargetLoc = pTarget->GetActorLocation();
	vAILoc.Z = 0.f;
	vTargetLoc.Z = 0.f;

	float fDist = FVector::Distance(vAILoc, vTargetLoc);

	// 사거리 체크
	return (fDist >= MinAttackDist && fDist <= MaxAttackDist);
}

void UBTDecorator_AttackDist::OnNodeActivation(FBehaviorTreeSearchData& SearchData)
{
	Super::OnNodeActivation(SearchData);

	LastExecutionTime = GetWorld()->GetTimeSeconds();
}


