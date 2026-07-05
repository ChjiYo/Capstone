#include "Mon_LarvaeAnim.h"
#include "Mon_Larvae.h"

UMon_LarvaeAnim::UMon_LarvaeAnim()
{
	eAnimType = EBaseAnim::Idle;
	eDir = EDir::Front;

	CurrentCombo = 1;
	NextAttackMax = 1;
	bIsComboInputOn = true;

	static ConstructorHelpers::FObjectFinder<UAnimMontage> NormalAttackAsset(TEXT("AnimMontage'/Game/UE_Asset/Character/Monster/Larvae/MTG_Larvae_Attack.MTG_Larvae_Attack'"));

	if (NormalAttackAsset.Succeeded())
	{
		m_pMTGNormalAttack = NormalAttackAsset.Object;
	}
}

void UMon_LarvaeAnim::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UMon_LarvaeAnim::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
}

void UMon_LarvaeAnim::InherAttackStartNotify()
{
	AMon_Larvae* pOwner = Cast<AMon_Larvae>(TryGetPawnOwner());

	if (IsValid(pOwner))
		pOwner->EnableWeaponCollision(true);
}

void UMon_LarvaeAnim::InherNextAttackEndNotify()
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

void UMon_LarvaeAnim::InherAttackEndNotify()
{
	CurrentCombo = 1;
	bIsComboInputOn = true;

	AMonster* pOwner = Cast<AMonster>(TryGetPawnOwner());
	if (IsValid(pOwner))
	{
		pOwner->AttackEnd();
	}

	AMon_Larvae* pLarvaeOwner = Cast<AMon_Larvae>(TryGetPawnOwner());
	if (IsValid(pLarvaeOwner))
	{
		pLarvaeOwner->EnableWeaponCollision(false);
	}
}
