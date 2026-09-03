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
					// Hit 몽타주 재생 중에는 넉백으로 생긴 순간적인 속도 때문에 eAnimType이
					// Run으로 덮어써지면 안 된다. 그대로 두면 Hit 몽타주가 블렌드 아웃되는
					// 순간 밑에 깔린 로코모션 상태가 Run이라 "Hit -> Run -> 다음 모션"처럼
					// 새어 보이는 원인이 된다 (의도된 흐름은 "Hit -> Idle -> 다음 모션")
					if (eAnimType != EBaseAnim::Hit)
					{
						eAnimType = EBaseAnim::Run;
					}
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
{
	eAnimType = EBaseAnim::Death;

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
