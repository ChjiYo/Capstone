#include "EffectDestroy.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"

AEffectDestroy::AEffectDestroy()
{
	PrimaryActorTick.bCanEverTick = true;

	Particle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Particle"));

    Particle->SetupAttachment(RootComponent);
}

void AEffectDestroy::SpawnVFX(USceneComponent* AttachComp, FName SocketName, const FString& strPath, const FString& strSoundPath, float fDuration)
{
    if (!IsValid(AttachComp))
        return;

    UNiagaraSystem* NiagaraAsset =
        LoadObject<UNiagaraSystem>(
            nullptr,
            *strPath);

    if (NiagaraAsset)
    {
        UNiagaraComponent* NiagaraComp =
            UNiagaraFunctionLibrary::SpawnSystemAttached(
                NiagaraAsset,
                AttachComp,
                SocketName,
                FVector::ZeroVector,
                FRotator::ZeroRotator,
                EAttachLocation::KeepRelativeOffset,
                true);

        if (NiagaraComp)
        {
            if (fDuration == 0.f)
            {
                NiagaraComp->SetAutoDestroy(true);
            }
            else
            {
                NiagaraComp->SetAutoDestroy(false);

                FTimerHandle TimerHandle;

                AttachComp->GetWorld()->GetTimerManager().SetTimer(
                    TimerHandle,
                    [NiagaraComp]()
                    {
                        if (NiagaraComp)
                        {
                            NiagaraComp->DestroyComponent();
                        }
                    },
                    fDuration,
                    false);
            }
        }

        USoundBase* SoundAsset =
            LoadObject<USoundBase>(
                nullptr,
                *strSoundPath);

        if (SoundAsset)
        {
            UGameplayStatics::PlaySoundAtLocation(
                AttachComp->GetWorld(),
                SoundAsset,
                AttachComp->GetComponentLocation());
        }
    }
}

void AEffectDestroy::SetEffectPath(const FString& strPath, const FString& strSoundPath)
{
	UParticleSystem* Asset = LoadObject<UParticleSystem>(nullptr, *strPath);

	if (IsValid(Asset))
		Particle->SetTemplate(Asset);

	Particle->OnSystemFinished.AddDynamic(this, &AEffectDestroy::Finish);

	USoundBase* SoundAsset = LoadObject<USoundBase>(nullptr, *strSoundPath);

	UGameplayStatics::PlaySoundAtLocation(GetWorld(), SoundAsset, GetActorLocation());
}

void AEffectDestroy::BeginPlay()
{
	Super::BeginPlay();
}

void AEffectDestroy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AEffectDestroy::Finish(UParticleSystemComponent* Component)
{
	Destroy();
}

void AEffectDestroy::InitializeEffect()
{


    if (ParticleTemplate)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ParticleTemplate, GetActorLocation(), GetActorRotation());
    }

    if (SoundTemplate)
    {
        UGameplayStatics::PlaySoundAtLocation(this, SoundTemplate, GetActorLocation());
    }
}