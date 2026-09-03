#include "MonsterAnim.h"
#include "Monster.h"
#include "GameFramework/CharacterMovementComponent.h"

UMonsterAnim::UMonsterAnim()
{

}

void UMonsterAnim::Hit()
{
    StopAllMontages(0.1f);

    SetAnimType(EBaseAnim::Hit);

    if (IsValid(m_pMTGHit))
    {
        float PlayTime = Montage_Play(m_pMTGHit);

        if (PlayTime > 0.f)
        {
            FOnMontageEnded EndDelegate;
            EndDelegate.BindUObject(this, &UMonsterAnim::OnHitMontageEnded);
            Montage_SetEndDelegate(EndDelegate, m_pMTGHit);
            return;
        }
    }

    // m_pMTGHit이 설정되어 있지 않거나 재생에 실패하면 OnHitMontageEnded가 호출될 계기가 없어
    // bHit이 영원히 true로 고착된다(=피격 이후 다시는 공격하지 못하는 "스턴이 안 풀리는" 버그).
    // 몽타주 없이도 즉시 상태를 되돌려 이 상태를 방지한다.
    ResetAfterHit();
}

void UMonsterAnim::AnimNotify_HitEnd()
{
    ResetAfterHit();
}

void UMonsterAnim::OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    ResetAfterHit();
}

void UMonsterAnim::ResetAfterHit()
{
    SetAnimType(EBaseAnim::Idle);

    AMonster* pMonster = Cast<AMonster>(TryGetPawnOwner());

    if (!IsValid(pMonster))
        return;

    pMonster->SetHit(false);

    // TakeDamage에서 받은 넉백(LaunchCharacter) 속도가 아직 남아 있으면, Hit이 끝나자마자
    // NativeUpdateAnimation이 그 속도를 보고 eAnimType을 곧바로 다시 Run으로 바꿔버려
    // "Hit -> Idle"이 아니라 "Hit -> Run"으로 보이게 된다. Hit이 끝나는 시점에 잔여 속도를
    // 지워서 확실히 Idle로 복귀하게 한다.
    if (UCharacterMovementComponent* pMovement = pMonster->GetCharacterMovement())
    {
        pMovement->StopMovementImmediately();
    }
}

