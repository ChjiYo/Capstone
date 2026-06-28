#include "AnimNotify_SpawnSkillEffect.h"
#include "EffectDestroy.h"
#include "Weapon.h"
#include "CharBase.h"

UAnimNotify_SpawnSkillEffect::UAnimNotify_SpawnSkillEffect()
{

}

void UAnimNotify_SpawnSkillEffect::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    if (!MeshComp || !EffectToSpawn) return;

    UWorld* World = MeshComp->GetWorld();
    AActor* Owner = MeshComp->GetOwner();
    FVector SpawnLocation = FVector::ZeroVector;
    FRotator SpawnRotation = FRotator::ZeroRotator;

    // 부착 대상을 추적하기 위한 변수 추가
    USceneComponent* TargetAttachComponent = nullptr;
    FName AttachSocketName = NAME_None;

    switch (TargetType)
    {
    case EEffectTarget::PlayerUnder:
    {
        ACharacter* pPlayer = UGameplayStatics::GetPlayerCharacter(World, 0);
        if (IsValid(pPlayer) && pPlayer->GetCapsuleComponent())
        {
            float HalfHeight = pPlayer->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
            SpawnLocation = pPlayer->GetActorLocation() - FVector(0, 0, HalfHeight);

            // 부착 대상: 플레이어의 캡슐 컴포넌트(루트)
            TargetAttachComponent = pPlayer->GetCapsuleComponent();
        }
        break;
    }
    case EEffectTarget::CasterUnder:
    {
        if (IsValid(Owner))
        {
            ACharacter* CharOwner = Cast<ACharacter>(Owner);
            if (CharOwner && CharOwner->GetCapsuleComponent())
            {
                float HalfHeight = CharOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
                SpawnLocation = Owner->GetActorLocation() - FVector(0, 0, HalfHeight);
                TargetAttachComponent = CharOwner->GetCapsuleComponent();
            }
            else
            {
                SpawnLocation = Owner->GetActorLocation();
                TargetAttachComponent = Owner->GetRootComponent();
            }
        }
        break;
    }
    case EEffectTarget::CasterSocket:
    {
        SpawnLocation = MeshComp->GetSocketLocation(SocketName);
        SpawnRotation = MeshComp->GetSocketRotation(SocketName); // 회전값도 가져오는 것이 좋습니다.

        // 부착 대상: 시전자의 스켈레탈 메시
        TargetAttachComponent = MeshComp;
        AttachSocketName = SocketName;
        break;
    }
    case EEffectTarget::WeaponSocket:
    {
        if (IsValid(Owner))
        {
            TArray<AActor*> AttachedActors;
            Owner->GetAttachedActors(AttachedActors);

            AWeapon* FoundWeapon = nullptr;
            for (AActor* AttachedActor : AttachedActors)
            {
                FoundWeapon = Cast<AWeapon>(AttachedActor);
                if (FoundWeapon) break;
            }

            if (FoundWeapon && FoundWeapon->GetWeaponMesh())
            {
                SpawnLocation = FoundWeapon->GetWeaponMesh()->GetSocketLocation(SocketName);
                SpawnRotation = FoundWeapon->GetWeaponMesh()->GetSocketRotation(SocketName);

                // 부착 대상: 무기의 메시
                TargetAttachComponent = FoundWeapon->GetWeaponMesh();
                AttachSocketName = SocketName;
            }
            else
            {
                SpawnLocation = MeshComp->GetSocketLocation(SocketName);
                SpawnRotation = MeshComp->GetSocketRotation(SocketName);

                // 무기가 없으면 기본 메시로 대체
                TargetAttachComponent = MeshComp;
                AttachSocketName = SocketName;
            }
        }
        break;
    }
    }

    if (IsValid(Owner))
    {
        FActorSpawnParameters params;
        params.Instigator = Cast<APawn>(Owner);
        params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        // 월드에 먼저 이펙트 스폰
        AEffectDestroy* Effect = World->SpawnActor<AEffectDestroy>(EffectToSpawn, SpawnLocation, SpawnRotation, params);

        if (IsValid(Effect))
        {
            // [추가된 로직] bAttachToTarget이 true이고, 타겟 컴포넌트가 존재한다면 부착 실행
            if (bAttachToTarget && IsValid(TargetAttachComponent))
            {
                // KeepWorldTransform을 사용하여 이미 계산된 위치(SpawnLocation)를 유지하면서 부착합니다.
                Effect->AttachToComponent(
                    TargetAttachComponent,
                    FAttachmentTransformRules::KeepWorldTransform,
                    AttachSocketName
                );
            }

            Effect->InitializeEffect();
        }
    }
}