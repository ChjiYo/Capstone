#include "BTDecorator_CheckHit.h"
#include "MonsterAIController.h"
#include "Monster.h"

UBTDecorator_CheckHit::UBTDecorator_CheckHit()
{
	NodeName = TEXT("Check Hit");
}

bool UBTDecorator_CheckHit::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	// 부모 클래스 검사
	if (!Super::CalculateRawConditionValue(OwnerComp, NodeMemory)) return false;

	AMonsterAIController* pController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());
	if (!IsValid(pController)) return false;

	AMonster* pMonster = Cast<AMonster>(pController->GetPawn());
	if (!IsValid(pMonster)) return false;

	// 몬스터의 현재 피격 상태(bHit)를 반환
	return pMonster->GetHit();
}