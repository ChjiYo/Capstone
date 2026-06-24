#pragma once

#include "EffectDestroy.h"
#include "Effect_Dot.generated.h"

UCLASS()
class CAPSTONE_API AEffect_Dot : public AEffectDestroy
{
	GENERATED_BODY()
	
public:
	AEffect_Dot();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* DamageSphere;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void ApplyDamageOverTime();

protected:
	FTimerHandle DamageTimerHandle;

	TSet<AActor*> OverlappingActors;

	UPROPERTY(EditAnywhere, Category = "Damage")
	float DamagePerTick = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Damage")
	float DamageInterval = 1.0f;

};
