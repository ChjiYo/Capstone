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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UProjectileMovementComponent* ProjectileMovement;

	UFUNCTION()	
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float ProjectileDamage;
};
