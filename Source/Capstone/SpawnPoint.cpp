#include "SpawnPoint.h"
#include "Monster.h"

ASpawnPoint::ASpawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;


	bSpawnEnable = true;
	SpawnTime = 1.f;
	SpawnDuration = 0.f;
	pSpawnMonster = nullptr;
	bInfinity = false;
}

void ASpawnPoint::Respawn()
{
	if (!IsValid(SpawnType))
		return;

	pSpawnMonster = nullptr;
	if (bInfinity)
		return;

	bSpawnEnable = true;
}

void ASpawnPoint::BeginPlay()
{
	Super::BeginPlay();
}

void ASpawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsValid(SpawnType) && bSpawnEnable)
	{
		SpawnDuration += DeltaTime;

		if (SpawnDuration >= SpawnTime)
		{
			SpawnDuration = 0.f;

			if (!bInfinity && bSpawnEnable)
			{
				bSpawnEnable = false;
			}

			FActorSpawnParameters params;
			params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			pSpawnMonster = GetWorld()->SpawnActor<AMonster>(SpawnType, GetActorLocation(), GetActorRotation(), params);

			pSpawnMonster->SetSpawnPoint(this);

		}
	}
}

