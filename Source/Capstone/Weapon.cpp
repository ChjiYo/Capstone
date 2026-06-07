#include "Weapon.h"
#include "Engine/DamageEvents.h"
#include "DrawDebugHelpers.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(RootComponent);

	CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionBox->SetBoxExtent(WeaponInfo.CollisionBoxSize);

}



void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	CollisionBox->SetCollisionProfileName(WeaponInfo.CollisionType);


	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::WepaonBeginOverlap);
	CollisionBox->OnComponentEndOverlap.AddDynamic(this, &AWeapon::WeaponEndOverlap);

	CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionBox->SetBoxExtent(WeaponInfo.CollisionBoxSize);
	
	
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeapon::LoadMesh(const FString& strPath)
{
	USkeletalMesh* mesh = LoadObject<USkeletalMesh>(nullptr, *strPath);

	if (IsValid(mesh))
		Mesh->SetSkeletalMesh(mesh);
}

void AWeapon::EnableWeaponCollision(bool bEnable)
{
	if (bEnable)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	else
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AWeapon::WepaonBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	FDamageEvent	DamageEvent;


	FName StartSocket = WeaponInfo.StartSocket;
	FName EndSocket = WeaponInfo.EndSocket;
		
	FVector vStart = CollisionBox->GetSocketLocation(StartSocket);
	FVector vEnd = CollisionBox->GetSocketLocation(EndSocket);

	FVector vDir = vEnd - vStart;
	vDir.Normalize();

	FCollisionQueryParams tColParams(NAME_None, false, this);
	TArray<FHitResult> HitArray;
	bool bCollision = GetWorld()->SweepMultiByProfile(HitArray, vStart, vEnd, vDir.ToOrientationQuat(), WeaponInfo.CollisionType, FCollisionShape::MakeBox(WeaponInfo.CollisionBoxSize),
		tColParams);


	FVector		vImpactPoint = FVector::ZeroVector;
	FVector		vImpactNormal = FVector::ZeroVector;


	for (auto hit : HitArray)
	{
		if (hit.GetActor() == OtherActor)
		{
			vImpactPoint = hit.ImpactPoint;
			vImpactNormal = hit.ImpactNormal;
			break;
		}
	}


	if (HitArray.Num() > 0)
		float fDamage = OtherActor->TakeDamage(WeaponInfo.BaseDamage, DamageEvent, this->GetInstigatorController(), this);

#if ENABLE_DRAW_DEBUG
	FVector vCenter = (vStart + vEnd) / 2.f;
	FColor	DrawColor = HitArray.Num() > 0 ? FColor::Yellow : FColor::Black;

	DrawDebugBox(GetWorld(), vCenter, FVector(15.0f, 50.f, 15.f), vDir.ToOrientationQuat(), DrawColor, false, 1.5f);

#endif

	FActorSpawnParameters params;
	params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
}

void AWeapon::WeaponEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}

