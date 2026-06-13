#include "Mon_NarbashAnim.h"
#include "Mon_Narbash.h"

UMon_NarbashAnim::UMon_NarbashAnim()
{
	eAnimType = EBaseAnim::Idle;
	eDir = EDir::Front;

	NextAttackMax = 3;


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

void UMon_NarbashAnim::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UMon_NarbashAnim::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	AMon_Narbash* pOnwer = Cast<AMon_Narbash>(TryGetPawnOwner());

	if (IsValid(pOnwer))
	{
		UCharacterMovementComponent* pMovement = pOnwer->GetCharacterMovement();

		if (IsValid(pMovement))
		{
			IsGround = pMovement->IsMovingOnGround();
			if (IsGround)
			{
				if (pMovement->Velocity.Size() > 0)
				{
					if (eAnimType == EBaseAnim::Attack)
					{
						InherAttackEndNotify();
					}

					eAnimType = EBaseAnim::Run;
					bMoveStop = false;
				}
				else
				{
					bMoveStop = true;
				}
			}
		}
	}
}

void UMon_NarbashAnim::ChangeAnim(EBaseAnim eAnim)
{
	eAnimType = eAnim;
}

void UMon_NarbashAnim::SkillSlotA()
{
	if (!Montage_IsPlaying(m_pMTGSkillSlotA))
			Montage_Play(m_pMTGSkillSlotA);
}

void UMon_NarbashAnim::SkillSlotB()
{
	if (!Montage_IsPlaying(m_pMTGSkillSlotB))
		Montage_Play(m_pMTGSkillSlotB);
}

void UMon_NarbashAnim::SkillSlotC()
{
	if (!Montage_IsPlaying(m_pMTGSkillSlotC))
		Montage_Play(m_pMTGSkillSlotC);
}

void UMon_NarbashAnim::AnimNotify_AttackStart()
{
	bNextAttackInputEnable = true;

	if (NextAttackIndex + 1 < NextAttackMax)
	{
		bNextAttack = true;
	}

	AMon_Narbash* pOwner = Cast<AMon_Narbash>(TryGetPawnOwner());

	if (IsValid(pOwner))
		pOwner->EnableWeaponCollision(true);
}

void UMon_NarbashAnim::AnimNotify_NextAttackEnd()
{
	bNextAttackInputEnable = false;

	if (bNextAttack)
	{
		bNextAttack = false;

		++NextAttackIndex;
	}
}

void UMon_NarbashAnim::AnimNotify_AttackEnd()
{
	bNextAttack = false;
	bNextAttackInputEnable = false;
	NextAttackIndex = 0;

	AMonster* pOwner = Cast<AMonster>(TryGetPawnOwner());
	AMon_Narbash* pNarbashOwner = Cast<AMon_Narbash>(TryGetPawnOwner());

	if (IsValid(pOwner))
	{
		pOwner->AttackEnd();
		
	}

	if (IsValid(pNarbashOwner))
	{
		pNarbashOwner->EnableWeaponCollision(false);
	}
}

void UMon_NarbashAnim::InherAttackStartNotify()
{
	
}

void UMon_NarbashAnim::InherNextAttackEndNotify()
{

}

void UMon_NarbashAnim::InherAttackEndNotify()
{
}
