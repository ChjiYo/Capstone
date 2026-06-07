#include "Py_GreyAnim.h"
#include "Py_Grey.h"

UPy_GreyAnim::UPy_GreyAnim()
{
	NextAttackMax = 4;

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
}

void UPy_GreyAnim::InherAttackEndNotify()
{
	APy_Grey* pOwner = Cast<APy_Grey>(TryGetPawnOwner());

	if (IsValid(pOwner))
		pOwner->EnableWeaponCollision(false);
}

void UPy_GreyAnim::InherReturnIdleNotify()
{
}
