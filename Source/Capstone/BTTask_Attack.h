#pragma once

#include "GameInfo.h"
#include "CharBaseAnim.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Attack.generated.h"

UENUM(BlueprintType)
enum class EAttackPlayType : uint8
{
	UseAnimState UMETA(DisplayName = "Use Anim State (Attack)"),
	UseMontage UMETA(DisplayName = "Use Anim Montage (Skill)")
};

UCLASS()
class CAPSTONE_API UBTTask_Attack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_Attack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory);
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack Options")
	EAttackPlayType PlayType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack Options", meta = (EditCondition = "PlayType == EAttackPlayType::UseAnimState"))
	EBaseAnim AttackAnimState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack Options", meta = (EditCondition = "PlayType == EAttackPlayType::UseMontage"))
	UAnimMontage* AttackMontage;

public:
	void AttackEnd();

private:
	bool bIsAttacking;
	
};
