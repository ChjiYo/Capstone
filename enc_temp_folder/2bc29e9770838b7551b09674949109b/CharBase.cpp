#include "CharBase.h"
#include "CharBaseAnim.h"
#include "Weapon.h"

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
	LOG(TEXT("HP : %d"), m_tInfo.iHp);


	if (m_tInfo.iHp <= 0)
	{
		UCharBaseAnim* pAnim = Cast<UCharBaseAnim>(GetMesh()->GetAnimInstance());

		pAnim->SetAnimType(EBaseAnim::Death);


	}



	return DamageAmount;
}
