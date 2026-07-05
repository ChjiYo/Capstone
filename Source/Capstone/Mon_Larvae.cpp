#include "Mon_Larvae.h"
#include "Mon_LarvaeAnim.h"
#include "Engine/DamageEvents.h"
#include "DrawDebugHelpers.h"

AMon_Larvae::AMon_Larvae()
{
	PrimaryActorTick.bCanEverTick = true;

	m_tInfo.iMaxHp = 100;
	m_tInfo.iHp = 100;
	m_tInfo.iAttack = 10;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("SkeletalMesh'/Game/Megaspikan/MegaspikanLarvae/Meshes/SK_MegaspikanLarvae.SK_MegaspikanLarvae'"));

	if (MeshAsset.Succeeded())
		GetMesh()->SetSkeletalMesh(MeshAsset.Object);

	static ConstructorHelpers::FClassFinder<UMon_LarvaeAnim> AnimAsset(TEXT("AnimBlueprint'/Game/UE_Asset/Character/Monster/Larvae/AP_Larvae.AP_Larvae_C'"));

	if (AnimAsset.Succeeded())
	{
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		GetMesh()->SetAnimInstanceClass(AnimAsset.Class);
	}

	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -100.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	WeaponBox = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponBox"));
	WeaponBox->SetupAttachment(GetMesh(), TEXT("Attack_Socket"));

	fTraceRange = 1000.f;
	fAttackDist = 500.f;
}

void AMon_Larvae::BeginPlay()
{
	Super::BeginPlay();

	pLarvaeAnim = Cast<UMon_LarvaeAnim>(GetMesh()->GetAnimInstance());

	WeaponBox->SetCollisionProfileName(TEXT("MonsterAttack"));

	WeaponBox->OnComponentBeginOverlap.AddDynamic(this, &AMon_Larvae::WepaonBeginOverlap);
	WeaponBox->OnComponentEndOverlap.AddDynamic(this, &AMon_Larvae::WeaponEndOverlap);

	WeaponBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WeaponBox->SetBoxExtent({ 200.0f, 100.f, 100.f });
}

void AMon_Larvae::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);	
}

void AMon_Larvae::EnableWeaponCollision(bool bEnable)
{
	if (bEnable)
	{
		WeaponBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	else
	{
		WeaponBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AMon_Larvae::WepaonBeginOverlap(UPrimitiveComponent * OverlappedComponent, AActor * OtherActor, UPrimitiveComponent * OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	FDamageEvent	DamageEvent;

	float fDamage = OtherActor->TakeDamage(m_tInfo.iAttack, DamageEvent, GetController(), this);

}

void AMon_Larvae::WeaponEndOverlap(UPrimitiveComponent * OverlappedComponent, AActor * OtherActor, UPrimitiveComponent * OtherComp, int32 OtherBodyIndex)
{
	LOG(TEXT("WeaponEndOverlap"));
}
