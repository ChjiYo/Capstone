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

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
