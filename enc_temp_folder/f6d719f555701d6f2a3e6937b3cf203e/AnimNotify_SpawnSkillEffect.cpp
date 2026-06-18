#include "AnimNotify_SpawnSkillEffect.h"
#include "EffectDestroy.h"

UAnimNotify_SpawnSkillEffect::UAnimNotify_SpawnSkillEffect()
{

}

void UAnimNotify_SpawnSkillEffect::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference); // Super 호출 필수

    if (!MeshComp || !EffectToSpawn) return;

    UWorld* World = MeshComp->GetWorld();
    AActor* Owner = MeshComp->GetOwner();
    FVector SpawnLocation = FVector::ZeroVector;

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
                  // 몬스터도 캐릭터 클래스라면 캡슐 높이를 빼서 발밑으로 보정
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
    }

    // 소환 로직
    if (IsValid(Owner))
    {
        FActorSpawnParameters params;
        params.Instigator = Cast<APawn>(Owner);
        params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        AEffectDestroy* Effect = World->SpawnActor<AEffectDestroy>(EffectToSpawn, SpawnLocation, FRotator::ZeroRotator, params);

        // 이펙트 초기화 함수 호출 (파티클/사운드 재생)
        if (IsValid(Effect))
        {
            Effect->InitializeEffect();
        }
    }
}