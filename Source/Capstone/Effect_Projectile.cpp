#include "Effect_Projectile.h"
#include "NiagaraComponent.h"

AEffect_Projectile::AEffect_Projectile()
{
	PrimaryActorTick.bCanEverTick = false;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));

	ProjectileMovement->InitialSpeed = 1500.f;
	ProjectileMovement->MaxSpeed = 1500.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;

    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionBox"));
    CollisionSphere->SetupAttachment(NiagaraComp);
}


void AEffect_Projectile::BeginPlay()
{
    Super::BeginPlay();
    

    CollisionSphere->SetCollisionProfileName(CollisionType);
 

    AActor* Player =  UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

    if (Player)
    {
        FVector Direction =
            (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal();

        ProjectileMovement->Velocity =  Direction * ProjectileMovement->InitialSpeed;
    }
}

void AEffect_Projectile::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
   NiagaraComp->Activate();
}