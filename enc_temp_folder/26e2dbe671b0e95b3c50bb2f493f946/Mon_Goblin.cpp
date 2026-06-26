#include "Mon_Goblin.h"
#include "Mon_GoblinAnim.h"
#include "Weapon.h"

AMon_Goblin::AMon_Goblin()
{
	PrimaryActorTick.bCanEverTick = true;

	m_tInfo.iMaxHp = 100;
	m_tInfo.iHp = 100;
	m_tInfo.iAttack = 10;
	
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -70.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	m_tRigging.m_pRightWeapon = nullptr;

	CharacterName = TEXT("Goblin");
}

void AMon_Goblin::BeginPlay()
{
	Super::BeginPlay();

	if (GoblinMeshAsset)
	{
		GetMesh()->SetSkeletalMesh(GoblinMeshAsset);
	}

	if (GoblinAnimClass)
	{
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		GetMesh()->SetAnimInstanceClass(GoblinAnimClass);
	}

	FActorSpawnParameters params;
	params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	m_tRigging.m_pRightWeapon = GetWorld()->SpawnActor<AWeapon>(FVector::ZeroVector, FRotator::ZeroRotator, params);
	m_tRigging.m_pRightWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("Damage_Socket"));

	if (!WeaponMeshPath.IsEmpty())
	{
		m_tRigging.m_pRightWeapon->LoadMesh(*WeaponMeshPath);
	}
}

void AMon_Goblin::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMon_Goblin::WepaonBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	
}

void AMon_Goblin::WeaponEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}
