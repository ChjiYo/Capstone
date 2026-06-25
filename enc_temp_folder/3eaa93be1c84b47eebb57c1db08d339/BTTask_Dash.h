#pragma once

#include "GameInfo.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Dash.generated.h"
UCLASS()
class CAPSTONE_API UBTTask_Dash : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_Dash();

protected:
	// 태스크가 실행될 때 호출되는 메인 함수
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

public:
	// 블랙보드에서 타겟(플레이어) 정보를 가져오기 위한 키
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector TargetKey;

	// 대쉬의 힘 (언리얼 에디터에서 몬스터마다 다르게 조절 가능)
	UPROPERTY(EditAnywhere, Category = "Dash")
	float DashPower = 2000.f;
	
};
