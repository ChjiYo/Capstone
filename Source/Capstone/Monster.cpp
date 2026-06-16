#include "Monster.h"

AMonster::AMonster()
{
	PrimaryActorTick.bCanEverTick = true;
	pSpawnPoint = nullptr;

	fTraceRange = 1000.f;
	fAttackDist = 200.f;
}

AMonster::~AMonster()
{
	if (pSpawnPoint)
		pSpawnPoint->Respawn();

	OnAttackEnd.Clear();
	AttackEndHandle.Reset(0);
}

void AMonster::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{

}

void AMonster::SetAttackEnd()
{
	if (OnAttackEnd.IsBound())
	{
		OnAttackEnd.Broadcast();
	}
}

void AMonster::BeginPlay()
{
	Super::BeginPlay();

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Monster"));
	GetCapsuleComponent()->SetNotifyRigidBodyCollision(true);
	
}

void AMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMonster::AttackEnd()
{
	OnAttackEnd.Broadcast();

	for (FDelegateHandle& handle : AttackEndHandle)
	{
		OnAttackEnd.Remove(handle);
	}
}
