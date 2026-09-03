#include "Mon_Bug.h"
#include "Mon_BugAnim.h"
#include "Engine/DamageEvents.h"

AMon_Bug::AMon_Bug()
{
	PrimaryActorTick.bCanEverTick = true;

	m_tInfo.iMaxHp = 100;
	m_tInfo.iHp = 100;
	m_tInfo.iAttack = 10;

	fTraceRange = 1000.f;
	fAttackDist = 200.f;

	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	WeaponBox = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponBox"));
	WeaponBox->SetupAttachment(GetMesh(), WeaponSocketName);
}

void AMon_Bug::BeginPlay()
{
	Super::BeginPlay();

	if (BugMeshAsset)
	{
		GetMesh()->SetSkeletalMesh(BugMeshAsset);
	}

	if (BugAnimClass)
	{
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		GetMesh()->SetAnimInstanceClass(BugAnimClass);
	}

	pBugAnim = Cast<UMon_BugAnim>(GetMesh()->GetAnimInstance());

	WeaponBox->SetCollisionProfileName(TEXT("MonsterAttack"));

	WeaponBox->OnComponentBeginOverlap.AddDynamic(this, &AMon_Bug::WepaonBeginOverlap);
	WeaponBox->OnComponentEndOverlap.AddDynamic(this, &AMon_Bug::WeaponEndOverlap);

	WeaponBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WeaponBox->SetBoxExtent({ 100.f, 100.f, 100.f });
}

void AMon_Bug::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMon_Bug::EnableWeaponCollision(bool bEnable)
{
	WeaponBox->SetCollisionEnabled(bEnable ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}

void AMon_Bug::WepaonBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	FDamageEvent DamageEvent;

	OtherActor->TakeDamage(m_tInfo.iAttack, DamageEvent, GetController(), this);
}

void AMon_Bug::WeaponEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}
