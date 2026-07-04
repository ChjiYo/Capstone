#pragma once

#include "GameInfo.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_CheckHit.generated.h"

UCLASS()
class CAPSTONE_API UBTDecorator_CheckHit : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_CheckHit();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	
};
