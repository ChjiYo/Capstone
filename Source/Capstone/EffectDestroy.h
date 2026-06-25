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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect")
	UNiagaraComponent* EffectComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	UNiagaraSystem* NiagaraTemplate;

	UPROPERTY(EditAnywhere, Category = "Effect")
	USoundBase* SoundTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	FName CollisionType;

public:
	static void SpawnVFX(
		USceneComponent* AttachComp, FName SocketName,UNiagaraSystem* NiagaraAsset, USoundBase* SoundAsset, float fDuration = 0.f);

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

public:
	UFUNCTION()
	void Finish(UParticleSystemComponent* Component);

public:
	virtual void InitializeEffect(float fDuration);
};
