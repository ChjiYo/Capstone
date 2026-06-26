#include "Mon_GoblinAnim.h"
#include "Mon_Goblin.h"
#include "Weapon.h"

UMon_GoblinAnim::UMon_GoblinAnim()
{
	eAnimType = EBaseAnim::Idle;
	eDir = EDir::Front;

	CurrentCombo = 1;
	NextAttackMax = 2;
	bIsComboInputOn = true;

	static ConstructorHelpers::FObjectFinder<UAnimMontage> NormalAttackAsset(TEXT("AnimMontage'/Game/UE_Asset/Character/Monster/Goblin/MTG_MonGoblin_Attack.MTG_MonGoblin_Attack'"));

	if (NormalAttackAsset.Succeeded())
	{
		m_pMTGNormalAttack = NormalAttackAsset.Object;
	}
	
}

void UMon_GoblinAnim::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UMon_GoblinAnim::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
}

void UMon_GoblinAnim::InherAttackStartNotify()
{
	AMonster* pOwner = Cast<AMonster>(TryGetPawnOwner());

	if (IsValid(pOwner))
	{
		pOwner->Get_Weapon(EWEAPON::RHAND)->EnableWeaponCollision(true);
	}
}

void UMon_GoblinAnim::InherNextAttackEndNotify()
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

void UMon_GoblinAnim::InherAttackEndNotify()
{
	CurrentCombo = 1;
	bIsComboInputOn = true;

	AMonster* pOwner = Cast<AMonster>(TryGetPawnOwner());

	if (IsValid(pOwner))
	{
		pOwner->AttackEnd();
		pOwner->Get_Weapon(EWEAPON::RHAND)->EnableWeaponCollision(false);
	}
}
