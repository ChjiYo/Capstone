#include "Effect_Projectile.h"
#include "NiagaraComponent.h"

AEffect_Projectile::AEffect_Projectile()
{
    PrimaryActorTick.bCanEverTick = false;

    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionBox"));
    RootComponent = CollisionSphere;

    EffectComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("EffectComp"));
    EffectComp->SetupAttachment(RootComponent);
    EffectComp->bAutoActivate = false;

    // 3. 발사체 무브먼트
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->UpdatedComponent = CollisionSphere;

	ProjectileMovement->InitialSpeed = 1500.f;
	ProjectileMovement->MaxSpeed = 1500.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;

    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}


void AEffect_Projectile::BeginPlay()
{
    Super::BeginPlay();
    

    CollisionSphere->SetCollisionProfileName(CollisionType);
    ProjectileMovement->Activate();

    AActor* Player =  UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

    if (Player)
    {
        if (Player)
        {
            FVector Direction = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal();

            ProjectileMovement->Velocity = Direction * ProjectileMovement->InitialSpeed;
        }
    }
}

void AEffect_Projectile::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
  
}