#include "CharBase.h"
#include "CharBaseAnim.h"
#include "Weapon.h"
#include "AIController.h"
#include "BrainComponent.h"

ACharBase::ACharBase()
{
	PrimaryActorTick.bCanEverTick = true;

	for (auto& Element : mapWeapon)
	{
		Element.Value = nullptr;
	}
}

void ACharBase::BeginPlay()
{
	Super::BeginPlay();
}

void ACharBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ACharBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ACharBase::Destroy()
{
	for (auto& iter : mapWeapon)
	{
		if (iter.Value != nullptr)
		{
			iter.Value->Destroy();
		}
	}
	Super::Destroy();
}

float ACharBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{

	m_tInfo.iHp -= Super:: TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	LOG(TEXT("Hit Character : %s"), *CharacterName);
	LOG(TEXT("HP : %d"), m_tInfo.iMaxHp);
	LOG(TEXT("HP : %d"), m_tInfo.iHp);




	if (m_tInfo.iHp <= 0)
	{
		UCharBaseAnim* pAnim = Cast<UCharBaseAnim>(GetMesh()->GetAnimInstance());
		if (pAnim)
		{
			// 공격 중이던 애니메이션 몽타주를 즉시 정지시킵니다.
			pAnim->StopAllMontages(0.1f);
			pAnim->Death();
		}

		AAIController* AIController = Cast<AAIController>(GetController());
		if (AIController && AIController->GetBrainComponent())
		{
			AIController->GetBrainComponent()->StopLogic("Monster is Dead");
		}

		if (GetCapsuleComponent())
		{
			GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}


		SetCanBeDamaged(false);


	}



	return DamageAmount;
}
