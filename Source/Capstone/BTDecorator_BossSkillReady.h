#pragma once

#include "GameInfo.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_BossSkillReady.generated.h"

// 나르바쉬가 고른 다음 스킬을 지금 바로 쓸 수 있는지(사거리/쿨다운/후딜/스턴 여부)를 검사한다.
// 조건 변화에 이벤트가 없으므로 AttackDist 데코레이터처럼 매 틱 재평가해서, 이미 실행 중인 낮은 우선순위
// 브랜치(접근 이동)를 Lower Priority Abort로 끊을 수 있게 한다
UCLASS()
class CAPSTONE_API UBTDecorator_BossSkillReady : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_BossSkillReady();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;
};
