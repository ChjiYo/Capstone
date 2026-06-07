#pragma once

#include "GameInfo.h"
#include "GameFramework/Actor.h"
#include "EffectDestroy.generated.h"

UCLASS()
class CAPSTONE_API AEffectDestroy : public AActor
{
	GENERATED_BODY()

public:
	AEffectDestroy();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UParticleSystemComponent* Particle;

public:
	static void SpawnVFX(
		USceneComponent* AttachComp,
		FName SocketName,
		const FString& strPath,
		const FString& strSoundPath,
		float fDuration = 0.f);

public:
	void SetEffectPath(const FString& strPathm,
		const FString& strSoundPath);



protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

public:
	UFUNCTION()
	void Finish(UParticleSystemComponent* Component);

};
