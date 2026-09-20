#pragma once

#include "GameInfo.h"
#include "GameFramework/Actor.h"
#include "BossProjectile.generated.h"

// 나르바쉬가 던지는 북채 투사체. 플레이어에게 닿으면 피해를 주고 터진다.
// 에셋(BP) 없이 파라곤 나르바쉬의 북채 메시/파티클을 코드에서 직접 불러 쓴다
UCLASS()
class CAPSTONE_API ABossProjectile : public AActor
{
	GENERATED_BODY()

public:
	ABossProjectile();

	// 발사 직후 호출. Damage는 최종 피해량, Speed는 cm/s, MaxTravel은 최대 비행 거리
	void Launch(const FVector& Direction, float Speed, float Damage, float MaxTravelDistance);

protected:
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	UPROPERTY(VisibleAnywhere)
	USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* StickMesh;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* TrailFX;

	UPROPERTY()
	UParticleSystem* HitCharacterFX = nullptr;

	UPROPERTY()
	UParticleSystem* HitWorldFX = nullptr;

	FVector Velocity = FVector::ZeroVector;
	float HitDamage = 20.f;
	float RemainingDistance = 2000.f;
	bool bExploded = false;

	void Explode(const FVector& Location, bool bHitCharacter);
};
