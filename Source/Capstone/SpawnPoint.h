#pragma once

#include "GameInfo.h"
#include "GameFramework/Actor.h"
#include "SpawnPoint.generated.h"

UCLASS()
class CAPSTONE_API ASpawnPoint : public AActor
{
	GENERATED_BODY()
	
public:	
	ASpawnPoint();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class AMonster> SpawnType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float SpawnTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	bool bInfinity;

	float SpawnDuration;
	bool bSpawnEnable;

	class AMonster* pSpawnMonster;

public:
	void Respawn();

	// 미니맵이 이 스폰포인트가 어떤 몬스터를 스폰하는지 알아야 처치 퀘스트 마커를 찍을 수 있으므로 노출
	TSubclassOf<class AMonster> GetSpawnType() const { return SpawnType; }

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
