#include "Mon_Narbash.h"
#include "Mon_NarbashAnim.h"
#include "Engine/DamageEvents.h"
#include "DrawDebugHelpers.h"

AMon_Narbash::AMon_Narbash()
{
	PrimaryActorTick.bCanEverTick = true;

	m_tInfo.iMaxHp = 100;
	m_tInfo.iHp = 100;
	m_tInfo.iAttack = 10;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("SkeletalMesh'/Game/ParagonNarbash/Characters/Heroes/Narbash/Meshes/Narbash.Narbash'"));

	if (MeshAsset.Succeeded())
		GetMesh()->SetSkeletalMesh(MeshAsset.Object);

	static ConstructorHelpers::FClassFinder<UMon_NarbashAnim> AnimAsset(TEXT("AnimBlueprint'/Game/UE_Asset/Character/Monster/Narbash/AP_Narbash.AP_Narbash_C'"));

	if (AnimAsset.Succeeded())
	{
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		GetMesh()->SetAnimInstanceClass(AnimAsset.Class);
	}

	
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -88.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	
	LeftWeaponBox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftWeaponBox"));
	LeftWeaponBox->SetupAttachment(GetMesh(), TEXT("weapon_head_l"));

	RightWeaponBox = CreateDefaultSubobject<UBoxComponent>(TEXT("RightWeaponBox"));
	RightWeaponBox->SetupAttachment(GetMesh(), TEXT("weapon_head_r"));

	DashSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DashSphere"));
	DashSphere->SetupAttachment(GetMesh(),TEXT("Chest"));

	fTraceRange = 1000.f;
	fAttackDist = 200.f;

}

void AMon_Narbash::BeginPlay()
{
	Super::BeginPlay();

	pNarbashAnim = Cast<UMon_NarbashAnim>(GetMesh()->GetAnimInstance());

	LeftWeaponBox->SetCollisionProfileName(TEXT("MonsterAttack"));
	RightWeaponBox->SetCollisionProfileName(TEXT("MonsterAttack"));
	DashSphere->SetCollisionProfileName(TEXT("MonsterAttack"));

	LeftWeaponBox->OnComponentBeginOverlap.AddDynamic(this, &AMon_Narbash::WepaonBeginOverlap);
	LeftWeaponBox->OnComponentEndOverlap.AddDynamic(this, &AMon_Narbash::WeaponEndOverlap);

	RightWeaponBox->OnComponentBeginOverlap.AddDynamic(this, &AMon_Narbash::WepaonBeginOverlap);
	RightWeaponBox->OnComponentEndOverlap.AddDynamic(this, &AMon_Narbash::WeaponEndOverlap);

	DashSphere->OnComponentBeginOverlap.AddDynamic(this, &AMon_Narbash::OnCollisionOverlap);

	LeftWeaponBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightWeaponBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DashSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	LeftWeaponBox->SetBoxExtent({ 15.0f, 50.f, 15.f });
	RightWeaponBox->SetBoxExtent({ 15.0f, 50.f, 15.f });
	DashSphere->SetSphereRadius(150.f);

}

void AMon_Narbash::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMon_Narbash::EnableWeaponCollision(bool bEnable)
{
	if (bEnable)
	{
		LeftWeaponBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		RightWeaponBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	else
	{
		LeftWeaponBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		RightWeaponBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		DashSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AMon_Narbash::ExecutePhysicalDash()
{
	DashSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	FVector DashDirection = GetActorForwardVector();
	LaunchCharacter(DashDirection * 8000.f, true, true);
}

void AMon_Narbash::OnCollisionOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	FDamageEvent	DamageEvent;
	OtherActor->TakeDamage(m_tInfo.iAttack * 3.f, DamageEvent, GetController(), this);
}

void AMon_Narbash::WepaonBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	FDamageEvent	DamageEvent;

	FName StartSocket;
	FName EndSocket;

	if (OverlappedComponent == LeftWeaponBox)
	{
		StartSocket = TEXT("FX_Weapon_L_Base");
		EndSocket = TEXT("FX_Weapon_L_Tip");
	}
	else if (OverlappedComponent == RightWeaponBox)
	{
		StartSocket = TEXT("FX_Weapon_R_Base");
		EndSocket = TEXT("FX_Weapon_R_Tip");
	}
	else
	{
		return;
	}

	FVector vStart = GetMesh()->GetSocketLocation(StartSocket);
	FVector vEnd = GetMesh()->GetSocketLocation(EndSocket);

	FVector vDir = vEnd - vStart;
	vDir.Normalize();

	FCollisionQueryParams tColParams(NAME_None, false, this);
	TArray<FHitResult> HitArray;
	bool bCollision = GetWorld()->SweepMultiByProfile(HitArray, vStart, vEnd, vDir.ToOrientationQuat(), TEXT("MonsterAttack"), FCollisionShape::MakeBox(FVector(15.0f, 50.f, 15.f)),
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

	if(HitArray.Num() > 0)
		float fDamage = OtherActor->TakeDamage(m_tInfo.iAttack, DamageEvent, GetController(), this);

#if ENABLE_DRAW_DEBUG
	FVector vCenter = (vStart + vEnd) / 2.f;
	FColor	DrawColor = HitArray.Num() > 0 ? FColor::Yellow : FColor::Black;

	DrawDebugBox(GetWorld(), vCenter, FVector(15.0f, 50.f, 15.f), vDir.ToOrientationQuat(), DrawColor, false, 1.5f);

#endif

	FActorSpawnParameters params;
	params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
}

void AMon_Narbash::WeaponEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}
