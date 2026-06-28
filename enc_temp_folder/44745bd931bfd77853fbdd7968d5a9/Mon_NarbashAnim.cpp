#include "Mon_NarbashAnim.h"
#include "Mon_Narbash.h"
#include "EffectDestroy.h"

UMon_NarbashAnim::UMon_NarbashAnim()
{
	eAnimType = EBaseAnim::Idle;
	eDir = EDir::Front;

	CurrentCombo = 1;
	NextAttackMax = 3;
	bIsComboInputOn = true;

	static ConstructorHelpers::FObjectFinder<UAnimMontage> NormalAttackAsset(TEXT("AnimMontage'/Game/UE_Asset/Character/Monster/Narbash/MTG_Narbash_Attack.MTG_Narbash_Attack'"));

	if (NormalAttackAsset.Succeeded())
	{
		m_pMTGNormalAttack = NormalAttackAsset.Object;
	}
}

void UMon_NarbashAnim::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UMon_NarbashAnim::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
}

void UMon_NarbashAnim::AnimNotify_DashStart()
{
	AMon_Narbash* pNarbashOwner = Cast<AMon_Narbash>(TryGetPawnOwner());

	if (IsValid(pNarbashOwner))
	{
		pNarbashOwner->ExecutePhysicalDash();
	}
}

void UMon_NarbashAnim::InherAttackStartNotify()
{
	AMon_Narbash* pOwner = Cast<AMon_Narbash>(TryGetPawnOwner());

	if (IsValid(pOwner))
		pOwner->EnableWeaponCollision(true);
}

void UMon_NarbashAnim::InherNextAttackEndNotify()
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

void UMon_NarbashAnim::InherAttackEndNotify()
{
	CurrentCombo = 1;
	bIsComboInputOn = true;

	AMonster* pOwner = Cast<AMonster>(TryGetPawnOwner());
	if (IsValid(pOwner))
	{
		pOwner->AttackEnd();
	}

	AMon_Narbash* pNarbashOwner = Cast<AMon_Narbash>(TryGetPawnOwner());
	if (IsValid(pNarbashOwner))
	{
		pNarbashOwner->EnableWeaponCollision(false);
	}
}
