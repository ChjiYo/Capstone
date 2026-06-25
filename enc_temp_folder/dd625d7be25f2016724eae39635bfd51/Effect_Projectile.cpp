#include "Effect_Projectile.h"
#include "NiagaraComponent.h"

AEffect_Projectile::AEffect_Projectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. 충돌체를 먼저 생성하고 루트 컴포넌트로 지정합니다.
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionBox"));
	RootComponent = CollisionSphere; // 핵심: 충돌체를 루트로 만듦
	// CollisionSphere->SetupAttachment(RootComponent); <-- 이 줄은 삭제하세요!

	// 2. 발사체 무브먼트 생성 및 설정
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 1500.f;
	ProjectileMovement->MaxSpeed = 1500.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;

	ProjectileMovement->UpdatedComponent = CollisionSphere;
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
  
}