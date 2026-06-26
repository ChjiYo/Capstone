#include "CharBase.h"
#include "Weapon.h"

ACharBase::ACharBase()
{
	PrimaryActorTick.bCanEverTick = true;
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

	Super::Destroy();
}

float ACharBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{

	m_tInfo.iHp -= Super:: TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	LOG(TEXT("Hit Character : %s"), *CharacterName);
	LOG(TEXT("HP : %d"), m_tInfo.iHp);


	if (m_tInfo.iHp <= 0)
	{
		LOG(TEXT("Death"));
		Destroy();
	}



	return DamageAmount;
}
