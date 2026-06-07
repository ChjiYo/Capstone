#include "Mon_Goblin.h"
#include "Mon_GoblinAnim.h"
#include "Weapon.h"

AMon_Goblin::AMon_Goblin()
{
	PrimaryActorTick.bCanEverTick = true;

	m_tInfo = { 100, 10 };

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("Blueprint'/Game/UE_Asset/Character/Monster/Goblin/Sword/BP_Goblin.BP_Goblin'"));

	if (MeshAsset.Succeeded())
		GetMesh()->SetSkeletalMesh(MeshAsset.Object);

	static ConstructorHelpers::FClassFinder<UMon_GoblinAnim> AnimAsset(TEXT("AnimBlueprint'/Game/UE_Asset/Character/Monster/Goblin/Sword/AP_Goblin.AP_Goblin_C'"));

	if (AnimAsset.Succeeded())
	{
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		GetMesh()->SetAnimInstanceClass(AnimAsset.Class);
	}

	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -70.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	m_tRigging.m_pRightWeapon = nullptr;

	fTraceRange = 1000.f;
	fAttackDist = 150.f;

	CharacterName = TEXT("Goblin");
}

void AMon_Goblin::BeginPlay()
{
	Super::BeginPlay();

	pGoblinAnim = Cast<UMon_GoblinAnim>(GetMesh()->GetAnimInstance());

	FActorSpawnParameters params;
	params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	m_tRigging.m_pRightWeapon = GetWorld()->SpawnActor<AWeapon>(FVector::ZeroVector, FRotator::ZeroRotator, params);

	m_tRigging.m_pRightWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("Damage_Socket"));

	m_tRigging.m_pRightWeapon->LoadMesh(TEXT("SkeletalMesh'/Game/Goblin/Mesh/Weapons/SK_Sword.SK_Sword'"));
}

void AMon_Goblin::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	m_tRigging.m_pRightWeapon->TraceWeapon();

}

void AMon_Goblin::EnableWeaponCollision(bool bEnable)
{
	if (m_tRigging.m_pRightWeapon)
	{
		m_tRigging.m_pRightWeapon->SetAttackState(bEnable);
	}
}

void AMon_Goblin::WepaonBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	
}

void AMon_Goblin::WeaponEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}
