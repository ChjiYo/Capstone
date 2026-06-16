#include "CharBaseAnim.h"
#include "Py_Grey.h"

UCharBaseAnim::UCharBaseAnim()
{
	eAnimType = EBaseAnim::Idle;
	eDir = EDir::Front;

	bMoveStop = true;
	bNextAttack = false;
	bNextAttackInputEnable = false;
	IsGround = true;
	NextAttackMax = 0;
	NextAttackIndex = 0;

}

void UCharBaseAnim::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UCharBaseAnim::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	APy_Grey* pOnwer = Cast<APy_Grey>(TryGetPawnOwner());

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
			else
			{
				if (eAnimType == EBaseAnim::Attack)
				{
					InherAttackEndNotify();
				}
			}
		}
	}

}

void UCharBaseAnim::NormalAttack()
{
	//eAnimType = EBaseAnim::Attack;
	//
	//if (bNextAttackInputEnable)
	//{
	//	bNextAttack = true;
	//}
	//	
	//bNextAttackInputEnable = true;
	
}

void UCharBaseAnim::JumpKey()
{
	eAnimType = EBaseAnim::Jump;
}

void UCharBaseAnim::AnimNotify_MoveStop()
{

	eAnimType = EBaseAnim::Idle;
	bMoveStop = true;

	InherMoveStopNotify();
}

void UCharBaseAnim::AnimNotify_AttackStart()
{
	InherAttackStartNotify();
}

void UCharBaseAnim::AnimNotify_NextAttackEnd()
{
	if (bNextAttack)
	{
		++NextAttackIndex;

		if (NextAttackIndex >= NextAttackMax)
			NextAttackIndex = 0;

		bNextAttack = false;
	}
	else
	{
		bNextAttackInputEnable = false;
	}

	InherNextAttackEndNotify();
}

void UCharBaseAnim::AnimNotify_AttackEnd()
{
	eAnimType = EBaseAnim::Idle;

	bNextAttackInputEnable = false;
	NextAttackIndex = 0;
	bNextAttack = false;

	InherAttackEndNotify();
}

void UCharBaseAnim::AnimNotify_ReturnIdle()
{
	eAnimType = EBaseAnim::Idle;

	bNextAttackInputEnable = false;
	NextAttackIndex = 0;
	bNextAttack = false;

	InherReturnIdleNotify();
}

void UCharBaseAnim::AnimNotify_SkillAStart()
{
}
