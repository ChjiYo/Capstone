#include "MonsterAnim.h"

UMonsterAnim::UMonsterAnim()
{

}

void UMonsterAnim::NormalAttack(bool bCombo)
{
	if (bCombo)
	{
		bNextAttack = true;
	}

	bNextAttackInputEnable = true;
}
