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
	void ChangeAnim(EBaseAnim _eAnim)
	{
		eAnimType = _eAnim;
	}

	void AttackReset()
	{
		if (NextAttackIndex >= NextAttackMax)
		{

			bNextAttack = false;
			bNextAttackInputEnable = false;
			NextAttackIndex = 0;
		}
	}

public:
	virtual void NormalAttack(bool bCombo);
	
};
