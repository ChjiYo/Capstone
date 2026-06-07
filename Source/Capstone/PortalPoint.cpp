#include "PortalPoint.h"
#include "Py_Grey.h"
#include "EffectDestroy.h"

APortalPoint::APortalPoint()
{
	PrimaryActorTick.bCanEverTick = true;

	m_pCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	RootComponent = m_pCollision;

	m_pParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Particle"));

}

void APortalPoint::BeginPlay()
{
	Super::BeginPlay();

	AEffectDestroy::SpawnVFX(
		GetRootComponent(),
		NAME_None,
		TEXT("NiagaraSystem'/Game/PortalSystem/FX/NS_Portal_02.NS_Portal_02'"),
		TEXT("SoundWave'/Game/UE_Asset/Sounds/Portal_Sound.Portal_Sound'"));
	
}

void APortalPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	m_pCollision->OnComponentBeginOverlap.AddDynamic(this, &APortalPoint::OnOverlapBegin);
}

void APortalPoint::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(Cast<APy_Grey>(OtherActor))
	{
		UGameplayStatics::OpenLevel(
			GetWorld(),
			TargetLevelName);
	}
}

