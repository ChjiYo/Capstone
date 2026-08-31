#pragma once

#include "GameInfo.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
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
    UPROPERTY(Category = "Niagara", VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    UNiagaraComponent* m_pNiagara;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    UNiagaraSystem* NiagaraTemplate;


    UPROPERTY(EditAnywhere)
    UBoxComponent* m_pCollision;

    UPROPERTY(Category = "Portal", EditAnywhere )
    FName TargetLevelName;

    UPROPERTY(Category = "Portal", EditAnywhere)
    FName RequiredQuestID;

    // 도착 레벨에서 스폰될 PlayerStart의 태그(PlayerStartTag). None이면 엔진 기본 선택(맵 중앙 등 첫 스폰용 PlayerStart)에 맡김
    UPROPERTY(Category = "Portal", EditAnywhere)
    FName TargetPlayerStartTag;

};

