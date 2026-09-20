#include "Mon_Bird.h"
#include "Mon_BirdAnim.h"
#include "Engine/DamageEvents.h"

AMon_Bird::AMon_Bird()
{
	PrimaryActorTick.bCanEverTick = true;

	m_tInfo.iMaxHp = 60;
	m_tInfo.iHp = 60;
	m_tInfo.iAttack = 12;
	DisplayName = FText::FromString(TEXT("새"));

	fTraceRange = 1000.f;
	fAttackDist = 200.f;

	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	WeaponBox = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponBox"));
	WeaponBox->SetupAttachment(GetMesh(), WeaponSocketName);



}

void AMon_Bird::BeginPlay()
{
	Super::BeginPlay();

	if (BirdMeshAsset)
	{
		GetMesh()->SetSkeletalMesh(BirdMeshAsset);
	}

	if (BirdAnimClass)
	{
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		GetMesh()->SetAnimInstanceClass(BirdAnimClass);
	}

	pBirdAnim = Cast<UMon_BirdAnim>(GetMesh()->GetAnimInstance());

	WeaponBox->SetCollisionProfileName(TEXT("MonsterAttack"));

	WeaponBox->OnComponentBeginOverlap.AddDynamic(this, &AMon_Bird::WepaonBeginOverlap);
	WeaponBox->OnComponentEndOverlap.AddDynamic(this, &AMon_Bird::WeaponEndOverlap);

	WeaponBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WeaponBox->SetBoxExtent({ 100.f, 100.f, 100.f });
}

void AMon_Bird::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMon_Bird::EnableWeaponCollision(bool bEnable)
{
	WeaponBox->SetCollisionEnabled(bEnable ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}

void AMon_Bird::WepaonBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	FDamageEvent DamageEvent;

	OtherActor->TakeDamage(m_tInfo.iAttack, DamageEvent, GetController(), this);
}

void AMon_Bird::WeaponEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}
