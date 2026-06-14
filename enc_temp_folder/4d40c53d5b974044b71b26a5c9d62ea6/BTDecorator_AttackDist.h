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
	virtual void OnNodeActivation(FBehaviorTreeSearchData& SearchData) override;
public:
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Condition")
		float MinAttackDist;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Condition")
		float MaxAttackDist;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Condition")
		float CooldownTime;

private:
	float LastExecutionTime;
};
