#include "CharBaseAnim.h"
#include "CharBase.h"

UCharBaseAnim::UCharBaseAnim()
{
	eAnimType = EBaseAnim::Idle;
	eDir = EDir::Front;

	bMoveStop = true;
	IsGround = true;
	NextAttackMax = 0;

}

void UCharBaseAnim::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UCharBaseAnim::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	ACharBase* pOnwer = Cast<ACharBase>(TryGetPawnOwner());

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

void UCharBaseAnim::JumpKey()
{
	eAnimType = EBaseAnim::Jump;
}

void UCharBaseAnim::Death()
{}

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
	InherNextAttackEndNotify();
}

void UCharBaseAnim::AnimNotify_AttackEnd()
{
	eAnimType = EBaseAnim::Idle;

	InherAttackEndNotify();
}

void UCharBaseAnim::AnimNotify_ReturnIdle()
{
	eAnimType = EBaseAnim::Idle;

	InherReturnIdleNotify();
}

void UCharBaseAnim::AnimNotify_SkillAStart()
{
	InherSkillAStartNotify();
}

void UCharBaseAnim::AnimNotify_Death()
{
	ACharBase* pOnwer = Cast<ACharBase>(TryGetPawnOwner());

	if(IsValid(pOnwer))
		pOnwer->Destroy();	

	InherDeathNotify();
}
