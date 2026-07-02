#pragma once

#include "CharBaseAnim.h"
#include "MonsterAnim.generated.h"

UCLASS()
class CAPSTONE_API UMonsterAnim : public UCharBaseAnim
{
	GENERATED_BODY()

public:
	UMonsterAnim();

public:
	void Hit();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* m_pMTGNormalAttack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* m_pMTGHit;

	UPROPERTY(EditDefaultsOnly, Category = "Dash|Animation")
	UAnimMontage* DashLeftMontage;
};
