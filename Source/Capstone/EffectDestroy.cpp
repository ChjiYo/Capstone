#include "EffectDestroy.h"
#include "NiagaraFunctionLibrary.h"


AEffectDestroy::AEffectDestroy()
{
    PrimaryActorTick.bCanEverTick = false;

    fLifeTime = 0.f;
}

void AEffectDestroy::SpawnVFX(USceneComponent* AttachComp, FName SocketName, UNiagaraSystem* NiagaraAsset, USoundBase* SoundAsset, float fDuration)
{
    if (!IsValid(AttachComp)) 
        return;

    if (NiagaraAsset)
    {
        UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
            NiagaraAsset,
            AttachComp,
            SocketName,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::KeepRelativeOffset,
            true
        );

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
                    false
                );
            }
        }
    }

    if (SoundAsset)
    {
        UGameplayStatics::PlaySoundAtLocation(
            AttachComp->GetWorld(),
            SoundAsset,
            AttachComp->GetComponentLocation()
        );
    }
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
    SpawnVFX(GetRootComponent(), NAME_None, NiagaraTemplate, SoundTemplate, fLifeTime);
}