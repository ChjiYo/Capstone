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

    switch (TargetType)
    {
          case EEffectTarget::PlayerUnder:
          {
              ACharacter* pPlayer = UGameplayStatics::GetPlayerCharacter(World, 0);
              if (IsValid(pPlayer) && pPlayer->GetCapsuleComponent())
              {
                  float HalfHeight = pPlayer->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
                  SpawnLocation = pPlayer->GetActorLocation() - FVector(0, 0, HalfHeight);
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
                  }
                  else
                  {
                      SpawnLocation = Owner->GetActorLocation();
                  }
              }
              break;
          }
          case EEffectTarget::CasterSocket:
          {
              SpawnLocation = MeshComp->GetSocketLocation(SocketName);
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
                    if (FoundWeapon)
                    {
                        break; 
                    }
                }
            
                if (FoundWeapon && FoundWeapon->GetWeaponMesh())
                {
                    SpawnLocation = FoundWeapon->GetWeaponMesh()->GetSocketLocation(SocketName);
                    SpawnRotation = FoundWeapon->GetWeaponMesh()->GetSocketRotation(SocketName);
                }
                else
                {
                    SpawnLocation = MeshComp->GetSocketLocation(SocketName);
                    SpawnRotation = MeshComp->GetSocketRotation(SocketName);
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

        AEffectDestroy* Effect = World->SpawnActor<AEffectDestroy>(EffectToSpawn, SpawnLocation, SpawnRotation, params);

        if (IsValid(Effect))
        {
            Effect->InitializeEffect();
        }
    }
}