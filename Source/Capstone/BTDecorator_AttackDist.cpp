#include "BTDecorator_AttackDist.h"
#include "MonsterAIController.h"
#include "Monster.h"

UBTDecorator_AttackDist::UBTDecorator_AttackDist()
{
	NodeName = TEXT("AttackDist");

	MinAttackDist = 0.f;
	MaxAttackDist = 200.f;

	// TickNode를 오버라이드했다고 자동으로 bNotifyTick이 켜지는 게 아니라 이 매크로를 호출해야
	// 실제로 매 틱 TickNode가 호출된다 (엔진 BTDecorator.h의 InitNotifyFlags 참고)
	INIT_DECORATOR_NODE_NOTIFY_FLAGS();
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

	FVector vAILoc = pMonster->GetActorLocation();
	FVector vTargetLoc = pTarget->GetActorLocation();
	vAILoc.Z = 0.f;
	vTargetLoc.Z = 0.f;

	float fDist = FVector::Distance(vAILoc, vTargetLoc);

	return (fDist >= MinAttackDist && fDist <= MaxAttackDist);
}

void UBTDecorator_AttackDist::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	// 거리 조건은 블랙보드 값 변경 같은 이벤트가 없으므로, 매 틱 재평가해서 Observer Aborts(Lower Priority)가
	// TargetMove처럼 이미 실행 중인 낮은 우선순위 브랜치를 실제로 인터럽트할 수 있게 해준다
	ConditionalFlowAbort(OwnerComp, EBTDecoratorAbortRequest::ConditionResultChanged);
}


