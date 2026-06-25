#pragma once

#include "GameInfo.h"
#include "EffectDestroy.h"
#include "Effect_Projectile.generated.h"

UCLASS()
class CAPSTONE_API AEffect_Projectile : public AEffectDestroy
{
	GENERATED_BODY()
	

public:
	AEffect_Projectile();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	UNiagaraSystem* HitNiagaraTemplate;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UProjectileMovementComponent* ProjectileMovement;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float ProjectileDamage;

	UFUNCTION()	
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

};
