#include "Effect_Projectile.h"
#include "NiagaraComponent.h"

AEffect_Projectile::AEffect_Projectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	RootComponent = CollisionSphere;
	CollisionSphere->SetSphereRadius(20.f);
	CollisionSphere->SetCollisionProfileName(TEXT("MonsterAttack"));

	if (Particle)
	{
		Particle->SetupAttachment(RootComponent);
	}

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionSphere;
	ProjectileMovement->InitialSpeed = 4500.f;
	ProjectileMovement->MaxSpeed = 4500.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.f;

	ProjectileDamage = 20.f;
	InitialLifeSpan = 8.0f;
}


void AEffect_Projectile::BeginPlay()
{
	Super::BeginPlay();

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AEffect_Projectile::OnOverlapBegin);

	if (MuzzleTemplate)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleTemplate, GetActorLocation(), GetActorRotation());
	}
}

void AEffect_Projectile::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this && OtherActor != GetInstigator() && OtherActor->GetOwner() != GetInstigator())
	{
		if (ImpactTemplate)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactTemplate, GetActorLocation(), GetActorRotation());
		}

		
	}
}