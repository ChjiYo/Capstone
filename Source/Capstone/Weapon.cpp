#include "Weapon.h"
#include "Engine/DamageEvents.h"
#include "DrawDebugHelpers.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

    AttackTraceChannel = ECC_Visibility;

    BaseDamage = 10.f;

}



void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeapon::SetAttackState(bool bActive)
{
    bIsAttacking = bActive;

    if (bIsAttacking)
    {
        DamagedActors.Empty();
    }
}

void AWeapon::LoadMesh(const FString& strPath)
{
	USkeletalMesh* mesh = LoadObject<USkeletalMesh>(nullptr, *strPath);

	if (IsValid(mesh))
		Mesh->SetSkeletalMesh(mesh);
}

void AWeapon::TraceWeapon()
{
    if (!bIsAttacking) return;

    FVector Start = Mesh->GetSocketLocation(TEXT("Weapon_Start_Socket"));
    FVector End = Mesh->GetSocketLocation(TEXT("Weapon_End_Socket"));
    FHitResult Hit;

    FCollisionQueryParams CollisionParams;
    CollisionParams.AddIgnoredActor(this);

    AActor* WeaponOwner = GetOwner();
    if (IsValid(WeaponOwner))
    {
        CollisionParams.AddIgnoredActor(WeaponOwner);
    }

    bool bResult = GetWorld()->LineTraceSingleByChannel(
        Hit,
        Start,
        End,
        AttackTraceChannel,
        CollisionParams);

    if (bResult)
    {
        AActor* HitActor = Hit.GetActor();

        if (IsValid(HitActor) && !DamagedActors.Contains(HitActor))
        {
            DamagedActors.Add(HitActor);

            FDamageEvent DamageEvent;
            AController* InstigatorController = WeaponOwner ? WeaponOwner->GetInstigatorController() : nullptr;

            HitActor->TakeDamage(
                BaseDamage,
                DamageEvent,
                InstigatorController,
                this);
        }
    }
}

