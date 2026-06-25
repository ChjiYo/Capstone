#include "EffectDestroy.h"
#include "NiagaraFunctionLibrary.h"


AEffectDestroy::AEffectDestroy()
{
    PrimaryActorTick.bCanEverTick = false;


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

void AEffectDestroy::InitializeEffect(float fDuration)
{   
    if (EffectComp && NiagaraTemplate)
    {
        // 이미 있는 컴포넌트에 에셋만 갈아끼우고 켭니다. (성능 최적화)
        EffectComp->SetAsset(NiagaraTemplate);
        EffectComp->Activate(true);

        // 수명(Duration) 설정이 있다면 액터 전체의 수명을 타이머로 설정
        // (투사체 자체가 사라지면 붙어있는 이펙트도 같이 사라집니다)
        if (fDuration > 0.f)
        {
            SetLifeSpan(fDuration);
        }
    }

    // 2. 사운드 재생
    if (SoundTemplate)
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            SoundTemplate,
            GetActorLocation()
        );
    }
}