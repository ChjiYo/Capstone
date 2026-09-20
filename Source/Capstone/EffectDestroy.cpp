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

                // 보호막처럼 지속시간이 끝나기 전에 액터가 먼저 파괴될 수 있으므로 날포인터 대신 WeakPtr로 잡는다
                TWeakObjectPtr<UNiagaraComponent> WeakNiagaraComp = NiagaraComp;

                AttachComp->GetWorld()->GetTimerManager().SetTimer(
                    TimerHandle,
                    [WeakNiagaraComp]()
                    {
                        if (WeakNiagaraComp.IsValid())
                        {
                            WeakNiagaraComp->DestroyComponent();

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
    // fLifeTime이 0 이하(설정을 깜빡한 경우)면 SetTimer(Rate<=0)가 조용히 무시되어 아래 버프 해제
    // 타이머가 아예 등록되지 않고, Niagara도 AutoDestroy(재생이 끝나야 파괴)로 스폰되어 루프 이펙트라면
    // 영원히 남는다. 버프를 부여하는 이펙트는 최소 지속시간을 보장해 이 문제를 막는다.
    float EffectiveLifeTime = fLifeTime;
    if (EffectiveLifeTime <= 0.f)
    {
        if (bGrantsShield)
        {
            EffectiveLifeTime = DefaultShieldDuration;
        }
        else if (bGrantsExplosiveBuff)
        {
            EffectiveLifeTime = DefaultExplosiveBuffDuration;
        }
    }

    SpawnVFX(GetRootComponent(), NAME_None, NiagaraTemplate, SoundTemplate, EffectiveLifeTime);

    if (bGrantsShield)
    {
        // 보호막 상태(수치/타이머)는 시전자가 들고 있고, 시전자가 이 액터를 직접 파괴해서 VFX를 걷는다
        if (ICombatInterface* ShieldTarget = Cast<ICombatInterface>(GetInstigator()))
        {
            ShieldTarget->ActivateShield(ShieldAmount, EffectiveLifeTime, this);
        }
    }


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
            }, EffectiveLifeTime, false);
    }
}