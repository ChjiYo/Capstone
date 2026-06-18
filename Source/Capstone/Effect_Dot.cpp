#include "Effect_Dot.h"

AEffect_Dot::AEffect_Dot()
{
	PrimaryActorTick.bCanEverTick = false;

	DamageSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DamageSphere"));

	if (RootComponent)
	{
		DamageSphere->SetupAttachment(RootComponent);
	}
	else
	{
		RootComponent = DamageSphere;
	}

	DamageSphere->SetSphereRadius(150.f);

	DamageSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void AEffect_Dot::BeginPlay()
{
	Super::BeginPlay();

	DamageSphere->OnComponentBeginOverlap.AddDynamic(this, &AEffect_Dot::OnOverlapBegin);
	DamageSphere->OnComponentEndOverlap.AddDynamic(this, &AEffect_Dot::OnOverlapEnd);

	GetWorld()->GetTimerManager().SetTimer(DamageTimerHandle, this, &AEffect_Dot::ApplyDamageOverTime, DamageInterval, true);
}

void AEffect_Dot::InitializeEffect()
{
	if (ParticleTemplate)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ParticleTemplate, GetActorLocation());
	}

	if (SoundTemplate)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SoundTemplate, GetActorLocation());
	}
}


void AEffect_Dot::OnOverlapBegin(UPrimitiveComponent * OverlappedComp, AActor * OtherActor, UPrimitiveComponent * OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	if (OtherActor && OtherActor->IsA(ACharacter::StaticClass()))
	{
		if (OtherActor == GetInstigator())
		{
			return; 
		}

		OverlappingActors.Add(OtherActor);
	}
}

void AEffect_Dot::OnOverlapEnd(UPrimitiveComponent * OverlappedComp, AActor * OtherActor, UPrimitiveComponent * OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor)
	{
		OverlappingActors.Remove(OtherActor);
	}
}

void AEffect_Dot::ApplyDamageOverTime()
{
	TArray<AActor*> ActorsToRemove;

	for (AActor* TargetActor : OverlappingActors)
	{
		if (IsValid(TargetActor)) 
		{
			UGameplayStatics::ApplyDamage(TargetActor, DamagePerTick, GetInstigatorController(), this, UDamageType::StaticClass());
		}
		else
		{
			ActorsToRemove.Add(TargetActor);
		}
	}

	for (AActor* DeadActor : ActorsToRemove)
	{
		OverlappingActors.Remove(DeadActor);
	}
}


