#include "PatrolPoint.h"

APatrolPoint::APatrolPoint()
{
	PrimaryActorTick.bCanEverTick = true;

}

void APatrolPoint::BeginPlay()
{
	Super::BeginPlay();
	
}

void APatrolPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

