#include "Mon_BirdAnim.h"
#include "Mon_Bird.h"

// 공격 몽타주(m_pMTGNormalAttack)에는 "Attack1"/"Attack2" 두 섹션이 있어야 하고,
// 피격 몽타주(m_pMTGHit)도 반드시 채워야 한다 - 비워두면 피격 후 다시는 공격하지 못하는
// 버그(bHit이 영원히 true로 고착)가 생긴다. 두 몽타주 모두 이 클래스를 부모로 하는
// 애니메이션 블루프린트(예: AP_Buff_Black)의 클래스 디폴트에서 지정한다.
UMon_BirdAnim::UMon_BirdAnim()
{
	eAnimType = EBaseAnim::Idle;
	eDir = EDir::Front;

	CurrentCombo = 1;
	NextAttackMax = 2;
	bIsComboInputOn = true;
	   
	static ConstructorHelpers::FObjectFinder<UAnimMontage> HitAsset(TEXT("/Script/Engine.AnimMontage'/Game/UE_Asset/Character/Monster/Bird/MTG_BirdHit.MTG_BirdHit'"));

	if (HitAsset.Succeeded())
	{
		m_pMTGHit = HitAsset.Object;
	}
}

void UMon_BirdAnim::InherAttackStartNotify()
{
	AMon_Bird* pOwner = Cast<AMon_Bird>(TryGetPawnOwner());

	if (IsValid(pOwner))
		pOwner->EnableWeaponCollision(true);
}

void UMon_BirdAnim::InherNextAttackEndNotify()
{
	bIsComboInputOn = (FMath::RandRange(1, 100) <= 60);

	if (bIsComboInputOn && (CurrentCombo < NextAttackMax))
	{
		CurrentCombo++;
		FName NextSection = *FString::Printf(TEXT("Attack%d"), CurrentCombo);
		Montage_JumpToSection(NextSection, m_pMTGNormalAttack);
	}
	else
	{
		bIsComboInputOn = false;
	}
}

void UMon_BirdAnim::InherAttackEndNotify()
{
	CurrentCombo = 1;
	bIsComboInputOn = true;

	AMonster* pOwner = Cast<AMonster>(TryGetPawnOwner());

	if (IsValid(pOwner))
		pOwner->AttackEnd();

	AMon_Bird* pBirdOwner = Cast<AMon_Bird>(TryGetPawnOwner());

	if (IsValid(pBirdOwner))
		pBirdOwner->EnableWeaponCollision(false);
}
