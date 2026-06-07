#pragma once

#include "GameInfo.h"
#include "GameFramework/Actor.h"
#include "PatrolPoint.generated.h"

UCLASS()
class CAPSTONE_API APatrolPoint : public AActor
{
	GENERATED_BODY()
	
public:	
	APatrolPoint();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
