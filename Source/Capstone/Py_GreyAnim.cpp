#include "Py_GreyAnim.h"
#include "Py_Grey.h"

UPy_GreyAnim::UPy_GreyAnim()
{
	eAnimType = EBaseAnim::Idle;
	eDir = EDir::Front;

	NextAttackMax = 4;
	CurrentCombo = 0;
	bIsComboInputOn = false;

	static ConstructorHelpers::FObjectFinder <UAnimMontage> SkillAAssetA(TEXT("AnimMontage'/Game/UE_Asset/Character/Player/Grey/MTG_PyGrey_SkillSlotA.MTG_PyGrey_SkillSlotA'"));

	if (SkillAAssetA.Succeeded())
	{
		m_pMTGSkillSlotA = SkillAAssetA.Object;
	}

	static ConstructorHelpers::FObjectFinder <UAnimMontage> SkillAAssetB(TEXT("AnimMontage'/Game/UE_Asset/Character/Player/Grey/MTG_PyGrey_SkillSlotB.MTG_PyGrey_SkillSlotB'"));

	if (SkillAAssetB.Succeeded())
	{
		m_pMTGSkillSlotB = SkillAAssetB.Object;
	}

	static ConstructorHelpers::FObjectFinder <UAnimMontage> SkillAAssetC(TEXT("AnimMontage'/Game/UE_Asset/Character/Player/Grey/MTG_PyGrey_SkillSlotC.MTG_PyGrey_SkillSlotC'"));

	if (SkillAAssetC.Succeeded())
	{
		m_pMTGSkillSlotC = SkillAAssetC.Object;
	}


	static ConstructorHelpers::FObjectFinder<UAnimMontage> NormalAttackAsset(TEXT("AnimMontage'/Game/UE_Asset/Character/Player/Grey/MTG_PyGrey_Attack.MTG_PyGrey_Attack'"));

	if (NormalAttackAsset.Succeeded())
	{
		m_pMTGNormalAttack = NormalAttackAsset.Object;
	}

}

void UPy_GreyAnim::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UPy_GreyAnim::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
}

void UPy_GreyAnim::SkillSlotA()
{
	if (!Montage_IsPlaying(m_pMTGSkillSlotA))
		Montage_Play(m_pMTGSkillSlotA);
}

void UPy_GreyAnim::SkillSlotB()
{
	if (!Montage_IsPlaying(m_pMTGSkillSlotB))
		Montage_Play(m_pMTGSkillSlotB);
}

void UPy_GreyAnim::SkillSlotC()
{
	if (!Montage_IsPlaying(m_pMTGSkillSlotC))
		Montage_Play(m_pMTGSkillSlotC);
}

void UPy_GreyAnim::NormalAttack()
{
	if (!Montage_IsPlaying(m_pMTGNormalAttack))
	{
		CurrentCombo = 1;
		bIsComboInputOn = false;

		Montage_Play(m_pMTGNormalAttack);
		Montage_JumpToSection(FName("Attack1"), m_pMTGNormalAttack);
	}
	else
	{
		bIsComboInputOn = true;
	}
}

void UPy_GreyAnim::InherMoveStopNotify()
{
}

void UPy_GreyAnim::InherAttackStartNotify()
{
	APy_Grey* pOwner = Cast<APy_Grey>(TryGetPawnOwner());

	if (IsValid(pOwner))
		pOwner->EnableWeaponCollision(true);
}

void UPy_GreyAnim::InherNextAttackEndNotify()
{
	if (bIsComboInputOn && CurrentCombo < NextAttackMax)
	{
		bIsComboInputOn = false;
		CurrentCombo++;         

		FName NextSection = *FString::Printf(TEXT("Attack%d"), CurrentCombo);

		Montage_JumpToSection(NextSection, m_pMTGNormalAttack);
	}
}

void UPy_GreyAnim::InherAttackEndNotify()
{
	APy_Grey* pOwner = Cast<APy_Grey>(TryGetPawnOwner());

	if (IsValid(pOwner))
		pOwner->EnableWeaponCollision(false);
}

void UPy_GreyAnim::InherReturnIdleNotify()
{
	CurrentCombo = 0;
	bIsComboInputOn = false;
}
