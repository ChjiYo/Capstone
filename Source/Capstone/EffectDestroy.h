#pragma once

#include "GameInfo.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "GameFramework/Actor.h"
#include "EffectDestroy.generated.h"


UCLASS()
class CAPSTONE_API AEffectDestroy : public AActor
{
	GENERATED_BODY()

public:
	AEffectDestroy();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	UNiagaraSystem* NiagaraTemplate;

	UPROPERTY(EditAnywhere, Category = "Effect")
	USoundBase* SoundTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	FName CollisionType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	float fLifeTime;

	// 이 이펙트가 시전자에게 폭발/화염 무기 버프(ICombatInterface::SetExplosiveBuffState)를 부여하는지 여부.
	// 폭발 스킬 전용 이펙트에서만 true로 설정할 것 — 보호막 등 다른 스킬 이펙트까지 이 버프를 켜버리는 버그가 있었음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	bool bGrantsExplosiveBuff = true;

public:
	 void SpawnVFX(USceneComponent* AttachComp, FName SocketName,UNiagaraSystem* NiagaraAsset, USoundBase* SoundAsset, float fDuration);
	 void SetEffectPath(const FString& strPath, const FString& strSoundPath);

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

public:
	UFUNCTION()
	void Finish(UParticleSystemComponent* Component);

public:
	virtual void InitializeEffect();
};
