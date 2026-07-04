#include "MonsterAnim.h"
#include "Monster.h"

UMonsterAnim::UMonsterAnim()
{

}

void UMonsterAnim::Hit()
{
    StopAllMontages(0.1f);

    if (IsValid(m_pMTGHit))
    {
        Montage_Play(m_pMTGHit);
    }
}

void UMonsterAnim::AnimNotify_HitEnd()
{
    AMonster* pMonster = Cast<AMonster>(TryGetPawnOwner());

    if (IsValid(pMonster))
        pMonster->SetHit(false);
}

