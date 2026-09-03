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
        float PlayTime = Montage_Play(m_pMTGHit);

        if (PlayTime > 0.f)
        {
            FOnMontageEnded EndDelegate;
            EndDelegate.BindUObject(this, &UMonsterAnim::OnHitMontageEnded);
            Montage_SetEndDelegate(EndDelegate, m_pMTGHit);
        }
    }
}

void UMonsterAnim::AnimNotify_HitEnd()
{
    AMonster* pMonster = Cast<AMonster>(TryGetPawnOwner());

    if (IsValid(pMonster))
        pMonster->SetHit(false);
}

void UMonsterAnim::OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    AMonster* pMonster = Cast<AMonster>(TryGetPawnOwner());

    if (IsValid(pMonster))
        pMonster->SetHit(false);
}

