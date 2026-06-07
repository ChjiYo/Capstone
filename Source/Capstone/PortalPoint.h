#pragma once

#include "GameInfo.h"
#include "GameFramework/Actor.h"
#include "PortalPoint.generated.h"

UCLASS()
class CAPSTONE_API APortalPoint : public AActor
{
	GENERATED_BODY()
	
public:	
	APortalPoint();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

protected:
    UFUNCTION()
    void OnOverlapBegin(
        UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

protected:
    UPROPERTY(Category = "Particle", VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    UParticleSystemComponent* m_pParticle;


    UPROPERTY(EditAnywhere)
    UBoxComponent* m_pCollision;

    UPROPERTY(Category = "Portal", EditAnywhere )
    FName TargetLevelName;

};

