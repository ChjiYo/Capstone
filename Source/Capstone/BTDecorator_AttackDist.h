#pragma once

#include "GameInfo.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_AttackDist.generated.h"

UCLASS()
class CAPSTONE_API UBTDecorator_AttackDist : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_AttackDist();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const;
	
};
