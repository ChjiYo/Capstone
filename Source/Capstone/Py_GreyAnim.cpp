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

	static ConstructorHelpers::FObjectFinder<UAnimSequence> SkillCSequenceAsset(TEXT("/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Attack_A_Med.Attack_A_Med"));

	if (SkillCSequenceAsset.Succeeded())
	{
		m_pSkillCSequence = SkillCSequenceAsset.Object;
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
	if (IsValid(m_pSkillCSequence))
	{
		// 몽타주(바이너리)를 새로 만들지 않고도 가로베기 시퀀스를 재생한다. 슬롯 이름은 기존 스킬 몽타주들이 쓰는 SlotA
		PlaySlotAnimationAsDynamicMontage(m_pSkillCSequence, TEXT("SlotA"), 0.05f, 0.15f, 1.f);
		return;
	}

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
	{
		pOwner->EnableWeaponCollision(true);

		// 스윙이 시작되는 순간 대상 쪽으로 파고든다. 이 노티파이는 스킬 몽타주에서도 울릴 수 있으므로 일반 공격 중일 때만
		if (Montage_IsPlaying(m_pMTGNormalAttack))
			pOwner->ApplyAttackAssist();
	}
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
