#include "Mon_GoblinAnim.h"
#include "Mon_Goblin.h"

UMon_GoblinAnim::UMon_GoblinAnim()
{
	eAnimType = EBaseAnim::Idle;
	eDir = EDir::Front;
}

void UMon_GoblinAnim::NativeInitializeAnimation()
{
}

void UMon_GoblinAnim::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	AMon_Goblin* pOnwer = Cast<AMon_Goblin>(TryGetPawnOwner());

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

void UMon_GoblinAnim::ChangeAnim(EBaseAnim eAnim)
{
	eAnimType = eAnim;
}

void UMon_GoblinAnim::AnimNotify_AttackStart()
{
	bNextAttackInputEnable = true;

	if (NextAttackIndex + 1 < NextAttackMax)
	{
		bNextAttack = true;
	}

	AMon_Goblin* pOwner = Cast<AMon_Goblin>(TryGetPawnOwner());

	if (IsValid(pOwner))
		pOwner->EnableWeaponCollision(true);
}

void UMon_GoblinAnim::AnimNotify_AttackEnd()
{
	bNextAttack = false;
	bNextAttackInputEnable = false;
	NextAttackIndex = 0;

	AMonster* pOwner = Cast<AMonster>(TryGetPawnOwner());
	AMon_Goblin* pNarbashOwner = Cast<AMon_Goblin>(TryGetPawnOwner());

	if (IsValid(pOwner))
	{
		pOwner->AttackEnd();

	}

	if (IsValid(pNarbashOwner))
	{
		pNarbashOwner->EnableWeaponCollision(false);
	}
}

void UMon_GoblinAnim::InherAttackStartNotify()
{
}

void UMon_GoblinAnim::InherAttackEndNotify()
{
}
