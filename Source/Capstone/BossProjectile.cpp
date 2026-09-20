#include "BossProjectile.h"
#include "BossCameraShake.h"
#include "Mon_Narbash.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "Py_Grey.h"

ABossProjectile::ABossProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetSphereRadius(55.f);
	CollisionSphere->SetCollisionProfileName(TEXT("MonsterAttack"));
	CollisionSphere->SetGenerateOverlapEvents(true);
	SetRootComponent(CollisionSphere);

	StickMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StickMesh"));
	StickMesh->SetupAttachment(CollisionSphere);
	StickMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StickMesh->SetCastShadow(false);
	StickMesh->SetRelativeScale3D(FVector(1.6f));

	TrailFX = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TrailFX"));
	TrailFX->SetupAttachment(CollisionSphere);
	TrailFX->bAutoActivate = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> StickAsset(TEXT("/Game/ParagonNarbash/FX/Meshes/Heroes/Narbash/SM_Narbash_Drumstick.SM_Narbash_Drumstick"));
	if (StickAsset.Succeeded())
		StickMesh->SetStaticMesh(StickAsset.Object);

	static ConstructorHelpers::FObjectFinder<UParticleSystem> TrailAsset(TEXT("/Game/ParagonNarbash/FX/Particles/Abilities/Throw/FX/P_Narbash_ThrowTrail.P_Narbash_ThrowTrail"));
	if (TrailAsset.Succeeded())
		TrailFX->SetTemplate(TrailAsset.Object);

	static ConstructorHelpers::FObjectFinder<UParticleSystem> HitCharAsset(TEXT("/Game/ParagonNarbash/FX/Particles/Abilities/Throw/FX/P_Narbash_Throw_HitCharacter.P_Narbash_Throw_HitCharacter"));
	if (HitCharAsset.Succeeded())
		HitCharacterFX = HitCharAsset.Object;

	static ConstructorHelpers::FObjectFinder<UParticleSystem> HitWorldAsset(TEXT("/Game/ParagonNarbash/FX/Particles/Abilities/Throw/FX/P_Narbash_Throw_HitWorld.P_Narbash_Throw_HitWorld"));
	if (HitWorldAsset.Succeeded())
		HitWorldFX = HitWorldAsset.Object;

	InitialLifeSpan = 6.f;
}

void ABossProjectile::Launch(const FVector& Direction, float Speed, float Damage, float MaxTravelDistance)
{
	Velocity = Direction.GetSafeNormal() * Speed;
	HitDamage = Damage;
	RemainingDistance = MaxTravelDistance;

	SetActorRotation(Velocity.Rotation());

	CollisionSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ABossProjectile::OnOverlapBegin);
}

void ABossProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bExploded)
		return;

	const FVector Step = Velocity * DeltaTime;

	// 벽/바닥에 막히면 그 자리에서 터진다 (플레이어 오버랩은 이벤트로 별도 처리)
	FHitResult Hit;
	FCollisionQueryParams Params(NAME_None, false, this);
	if (AActor* OwnerActor = GetInstigator())
		Params.AddIgnoredActor(OwnerActor);

	if (GetWorld()->SweepSingleByChannel(Hit, GetActorLocation(), GetActorLocation() + Step, FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeSphere(30.f), Params))
	{
		Explode(Hit.ImpactPoint, false);
		return;
	}

	AddActorWorldOffset(Step);

	// 북채가 날아가면서 빙글빙글 돈다
	StickMesh->AddLocalRotation(FRotator(0.f, 0.f, 900.f * DeltaTime));

	RemainingDistance -= Step.Size();

	if (RemainingDistance <= 0.f)
		Explode(GetActorLocation(), false);
}

void ABossProjectile::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bExploded || !IsValid(OtherActor) || OtherActor == GetInstigator())
		return;

	// 플레이어에게만 피해를 준다 (다른 몬스터/NPC는 무시)
	if (!OtherActor->IsA<APy_Grey>())
		return;

	FDamageEvent DamageEvent;
	const float Dealt = OtherActor->TakeDamage(HitDamage, DamageEvent, GetInstigatorController(), GetInstigator());

	if (AMon_Narbash* Boss = Cast<AMon_Narbash>(GetInstigator()))
		Boss->NotifyProjectileResult(Dealt > 0.f);

	if (Dealt > 0.f)
	{
		if (ACharacter* Victim = Cast<ACharacter>(OtherActor))
			Victim->LaunchCharacter(Velocity.GetSafeNormal2D() * 450.f + FVector(0.f, 0.f, 220.f), true, true);

		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
			PC->ClientStartCameraShake(UBossShake_Heavy::StaticClass(), 0.8f);
	}

	Explode(GetActorLocation(), true);
}

void ABossProjectile::Explode(const FVector& Location, bool bHitCharacter)
{
	if (bExploded)
		return;

	bExploded = true;

	// 플레이어에게 닿지 못하고 터졌다면 "회피당함"으로 적응형 학습에 보고 (명중은 OnOverlapBegin에서 보고)
	if (!bHitCharacter)
	{
		if (AMon_Narbash* Boss = Cast<AMon_Narbash>(GetInstigator()))
			Boss->NotifyProjectileResult(false);
	}

	UParticleSystem* Effect = bHitCharacter ? HitCharacterFX : HitWorldFX;
	if (Effect)
		UGameplayStatics::SpawnEmitterAtLocation(this, Effect, Location, FRotator::ZeroRotator, FVector(1.4f));

	Destroy();
}
