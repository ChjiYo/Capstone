#pragma once

#include "GameInfo.h"
#include "AIController.h"
#include "MonsterAIController.generated.h"

UCLASS()
class CAPSTONE_API AMonsterAIController : public AAIController
{
	GENERATED_BODY()

public:
	AMonsterAIController();

protected:
	UBehaviorTree* m_pBTAsset;
	UBlackboardData* m_pBBAsset;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

public:
	void SetTarget(UObject* pTarget);
	ACharacter* GetTarget() const;

	
};
