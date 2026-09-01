#include "EffectDestroy.h"
#include "CombatInterface.h"
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
            EffectScale,
            EAttachLocation::KeepRelativeOffset,
            true,
            ENCPoolMethod::None
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


    if (!bGrantsExplosiveBuff)
        return;

    AActor* EffectOwner = GetInstigator();

    // 이 주인이 인터페이스를 장착하고 있는지(SetExplosiveBuffState 기능이 있는지) 확인
    ICombatInterface* CombatTarget = Cast<ICombatInterface>(EffectOwner);
    if (CombatTarget)
    {
        // 1. 이펙트 생성 시점: 버프를 ON으로 설정
        CombatTarget->SetExplosiveBuffState(true);
    
        // 캐릭터가 죽어서 사라지는 등 예외 처리를 위해 Weak Pointer 사용
        TWeakObjectPtr<AActor> WeakOwner = EffectOwner;
    
        // 2. LifeTime이 지났을 때: 버프를 OFF로 설정하는 타이머 실행
        FTimerHandle BuffTimerHandle;
        GetWorld()->GetTimerManager().SetTimer(BuffTimerHandle, [WeakOwner]()
            {
                if (WeakOwner.IsValid()) // 주인이 아직 살아있다면
                {
                    if (ICombatInterface* Target = Cast<ICombatInterface>(WeakOwner.Get()))
                    {
                        Target->SetExplosiveBuffState(false);
                    }
                }
            }, fLifeTime, false);
    }
}