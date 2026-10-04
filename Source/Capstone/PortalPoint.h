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

    // 마을로 돌아가는 길 등 미니맵에 표시하고 싶은 포탈에만 체크. 기본은 꺼짐(보스 게이트 등은 굳이 안 보여줘도 됨)
    UPROPERTY(Category = "Portal", EditAnywhere)
    bool bShowOnMinimap = false;

public:
    bool ShouldShowOnMinimap() const { return bShowOnMinimap; }

    // 이 포탈이 보내주는 레벨 이름 (퀘스트 목표 레벨까지 가는 길 안내에 쓴다)
    FName GetTargetLevelName() const { return TargetLevelName; }

    // None이 아니면 그 퀘스트를 진행 중일 때만 통과되는 게이트 포탈
    FName GetRequiredQuestID() const { return RequiredQuestID; }

};

