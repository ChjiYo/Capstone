#include "Mon_GolemAnim.h"
#include "Mon_Golem.h"

UMon_GolemAnim::UMon_GolemAnim()
{
	eAnimType = EBaseAnim::Idle;
	eDir = EDir::Front;

	CurrentCombo = 1;
	NextAttackMax = 6;
	bIsComboInputOn = true;

	static ConstructorHelpers::FObjectFinder<UAnimMontage> HitAsset(TEXT("/Script/Engine.AnimMontage'/Game/UE_Asset/Character/Monster/Golem/MTG_GolemHit.MTG_GolemHit'"));

	if (HitAsset.Succeeded())
	{
		m_pMTGHit = HitAsset.Object;
	}
}

void UMon_GolemAnim::InherAttackStartNotify()
{
	AMon_Golem* pOwner = Cast<AMon_Golem>(TryGetPawnOwner());
	if (!IsValid(pOwner))
		return;

	FName CurrentSection = Montage_GetCurrentSection(m_pMTGNormalAttack);

	if (CurrentSection == RightAttackSection)
	{
		bLastAttackWasLeft = false;
		pOwner->BeginAttack(EGolemAttackType::Right);
	}
	else if (CurrentSection == HeavyAttackSection)
	{
		// 강공격은 왼/오른 교대 순서(bLastAttackWasLeft)를 건드리지 않는다
		pOwner->BeginAttack(EGolemAttackType::Heavy);
	}
	else
	{
		bLastAttackWasLeft = true;
		pOwner->BeginAttack(EGolemAttackType::Left);
	}
}

void UMon_GolemAnim::InherNextAttackEndNotify()
{
	if (CurrentCombo >= NextAttackMax)
		return;

	AMon_Golem* pOwner = Cast<AMon_Golem>(TryGetPawnOwner());
	if (!IsValid(pOwner))
		return;

	CurrentCombo++;

	if (FMath::FRand() <= pOwner->CritRate)
	{
		Montage_JumpToSection(HeavyAttackSection, m_pMTGNormalAttack);
	}
	else
	{
		FName NextSection = bLastAttackWasLeft ? RightAttackSection : LeftAttackSection;
		Montage_JumpToSection(NextSection, m_pMTGNormalAttack);
	}
}

void UMon_GolemAnim::InherAttackEndNotify()
{
	CurrentCombo = 1;
	bLastAttackWasLeft = false;

	AMon_Golem* pOwner = Cast<AMon_Golem>(TryGetPawnOwner());

	if (IsValid(pOwner))
	{
		pOwner->AttackEnd();
		pOwner->EndAttack();
	}
}
