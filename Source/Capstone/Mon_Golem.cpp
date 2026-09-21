#include "Mon_Golem.h"
#include "Mon_GolemAnim.h"
#include "Engine/DamageEvents.h"

AMon_Golem::AMon_Golem()
{
	PrimaryActorTick.bCanEverTick = true;

	m_tInfo.iMaxHp = 200;
	m_tInfo.iHp = 200;
	m_tInfo.iAttack = 20;
	DisplayName = FText::FromString(TEXT("골렘"));

	// 공격/피격음은 골렘 애니메이션(Buff_Red)의 노티파이에 이미 연결돼 있어서 비워둔다
	SFXProfile.Alert = ESFX::Mon_Alert_Big;
	SFXProfile.Attack = ESFX::None;
	SFXProfile.Hit = ESFX::None;
	SFXProfile.Death = ESFX::Mon_Death_Big;
	SFXProfile.Step = ESFX::Mon_Step_Heavy;
	SFXProfile.StepStride = 240.f;
	SFXProfile.bArmored = true;

	fTraceRange = 800.f;
	fAttackDist = 250.f;

	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	LeftWeaponBox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftWeaponBox"));
	LeftWeaponBox->SetupAttachment(GetMesh(), LeftHandSocketName);

	RightWeaponBox = CreateDefaultSubobject<UBoxComponent>(TEXT("RightWeaponBox"));
	RightWeaponBox->SetupAttachment(GetMesh(), RightHandSocketName);
}

void AMon_Golem::BeginPlay()
{
	Super::BeginPlay();

	if (GolemMeshAsset)
	{
		GetMesh()->SetSkeletalMesh(GolemMeshAsset);
	}

	if (GolemAnimClass)
	{
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		GetMesh()->SetAnimInstanceClass(GolemAnimClass);
	}

	pGolemAnim = Cast<UMon_GolemAnim>(GetMesh()->GetAnimInstance());

	LeftWeaponBox->SetCollisionProfileName(TEXT("MonsterAttack"));
	RightWeaponBox->SetCollisionProfileName(TEXT("MonsterAttack"));

	LeftWeaponBox->OnComponentBeginOverlap.AddDynamic(this, &AMon_Golem::WepaonBeginOverlap);
	LeftWeaponBox->OnComponentEndOverlap.AddDynamic(this, &AMon_Golem::WeaponEndOverlap);

	RightWeaponBox->OnComponentBeginOverlap.AddDynamic(this, &AMon_Golem::WepaonBeginOverlap);
	RightWeaponBox->OnComponentEndOverlap.AddDynamic(this, &AMon_Golem::WeaponEndOverlap);

	LeftWeaponBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightWeaponBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	LeftWeaponBox->SetBoxExtent({ 20.f, 20.f, 40.f });
	RightWeaponBox->SetBoxExtent({ 20.f, 20.f, 40.f });
}

void AMon_Golem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMon_Golem::BeginAttack(EGolemAttackType AttackType)
{
	CurrentAttackType = AttackType;
	HitActorsThisAttack.Empty();

	const bool bUseLeft = (AttackType == EGolemAttackType::Left || AttackType == EGolemAttackType::Heavy);
	const bool bUseRight = (AttackType == EGolemAttackType::Right || AttackType == EGolemAttackType::Heavy);

	LeftWeaponBox->SetCollisionEnabled(bUseLeft ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	RightWeaponBox->SetCollisionEnabled(bUseRight ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}

void AMon_Golem::EndAttack()
{
	LeftWeaponBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightWeaponBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AMon_Golem::WepaonBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsValid(OtherActor) || HitActorsThisAttack.Contains(OtherActor))
		return;

	HitActorsThisAttack.Add(OtherActor);

	FDamageEvent DamageEvent;

	float fDamage = m_tInfo.iAttack;

	if (CurrentAttackType == EGolemAttackType::Heavy)
		fDamage *= HeavyAttackDamageMultiplier;

	OtherActor->TakeDamage(fDamage, DamageEvent, GetController(), this);
}

void AMon_Golem::WeaponEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}
