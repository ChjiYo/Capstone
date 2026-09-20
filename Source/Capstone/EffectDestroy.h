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

	// 나이아가라 이펙트 크기 배율. 1보다 작게 주면 축소, 크게 주면 확대됨 (에셋 자체를 건드리지 않고 이 BP에서만 조절)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	FVector EffectScale = FVector(1.f, 1.f, 1.f);

	// 이 이펙트가 시전자에게 폭발/화염 무기 버프(ICombatInterface::SetExplosiveBuffState)를 부여하는지 여부.
	// 폭발 스킬 전용 이펙트에서만 true로 설정할 것 — 보호막 등 다른 스킬 이펙트까지 이 버프를 켜버리는 버그가 있었음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	bool bGrantsExplosiveBuff = true;

	// bGrantsExplosiveBuff인 이펙트인데 fLifeTime을 0(기본값) 그대로 둔 경우 사용할 기본 버프 지속시간(초).
	// fLifeTime<=0이면 FTimerManager::SetTimer가 타이머 등록 자체를 조용히 무시해서 버프 해제 타이머가
	// 영원히 실행되지 않는 버그가 있었음 — 검불꽃 스킬(SwordAuror)이 계속 유지되던 원인이 이것.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	float DefaultExplosiveBuffDuration = 10.f;

	// 이 이펙트가 시전자에게 보호막(ICombatInterface::ActivateShield)을 부여하는지 여부. 보호막 스킬(스킬 B) 전용 이펙트에서만 true.
	// 보호막의 지속시간은 fLifeTime을 그대로 따르고, 시전자가 보호막이 깨지는 순간 이 이펙트 액터를 파괴해서 VFX도 함께 사라진다.
	// 이 값을 true로 둘 때는 bGrantsExplosiveBuff는 반드시 false로 둘 것
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Shield")
	bool bGrantsShield = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Shield", meta = (EditCondition = "bGrantsShield", ClampMin = "1"))
	int32 ShieldAmount = 50;

	// fLifeTime을 0(기본값) 그대로 둔 경우 사용할 보호막 지속시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Shield", meta = (EditCondition = "bGrantsShield", ClampMin = "0.1"))
	float DefaultShieldDuration = 10.f;

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
