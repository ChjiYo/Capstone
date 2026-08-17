#include "Mon_Narbash.h"
#include "Mon_NarbashAnim.h"
#include "Engine/DamageEvents.h"
#include "DrawDebugHelpers.h"
#include "MonsterAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraShakeBase.h"
#include "UI_BossHpBar.h"
#include "CombatPatternSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Components/WidgetComponent.h"
#include "Py_Grey.h"
#include "StoryComponent.h"

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

	// 보스는 머리 위 HP바 대신 화면 상단 전용 보스 HP바를 사용
	if (IsValid(HpBarComponent))
		HpBarComponent->SetVisibility(false);

	if (IsValid(BossHpBarClass))
	{
		BossHpBarWidget = CreateWidget<UUI_BossHpBar>(GetWorld(), BossHpBarClass);

		if (IsValid(BossHpBarWidget))
		{
			BossHpBarWidget->AddToViewport();
			BossHpBarWidget->SetBossHp(GetHpRatio());
			BossHpBarWidget->SetPhase(CurrentPhase);
		}
	}
}

void AMon_Narbash::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(BossHpBarWidget))
	{
		BossHpBarWidget->RemoveFromParent();
		BossHpBarWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AMon_Narbash::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

float AMon_Narbash::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 페이즈 전환 연출 중에는 무적
	if (bPhaseTransitioning)
		return 0.f;

	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	float HpRatio = GetHpRatio();

	if (IsValid(BossHpBarWidget))
		BossHpBarWidget->SetBossHp(HpRatio);

	if (CurrentPhase == 1 && HpRatio > 0.f && HpRatio <= Phase2HpRatio)
	{
		EnterPhase2();
	}

	return ActualDamage;
}

void AMon_Narbash::EnterPhase2()
{
	CurrentPhase = 2;
	bPhaseTransitioning = true;

	if (Phase2AttackPatterns.Num() > 0)
	{
		AttackPatterns = Phase2AttackPatterns;
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed *= Phase2SpeedMultiplier;
	}

	if (IsValid(pNarbashAnim) && IsValid(PhaseTransitionMontage))
	{
		pNarbashAnim->Montage_Play(PhaseTransitionMontage, 1.f);
	}

	if (PhaseTransitionCameraShake)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			PC->ClientStartCameraShake(PhaseTransitionCameraShake);
		}
	}

	if (AMonsterAIController* AICon = Cast<AMonsterAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AICon->GetBlackboardComponent())
		{
			BB->SetValueAsInt(TEXT("Phase"), CurrentPhase);
		}
	}

	if (IsValid(BossHpBarWidget))
	{
		BossHpBarWidget->SetPhase(CurrentPhase);
	}

	if (APy_Grey* Player = Cast<APy_Grey>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		if (Player->StoryComponent && Phase2DialogueLines.Num() > 0)
			Player->StoryComponent->PlayDialogue(Phase2DialogueLines, false);
	}

	GetWorldTimerManager().SetTimer(PhaseTransitionTimerHandle, this, &AMon_Narbash::EndPhaseTransition, PhaseTransitionInvincibleDuration, false);
}

void AMon_Narbash::EndPhaseTransition()
{
	bPhaseTransitioning = false;
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

	if (HitArray.Num() > 0)
	{
		float fDamage = OtherActor->TakeDamage(m_tInfo.iAttack, DamageEvent, GetController(), this);

		// 7단계 적응형 AI 학습에 이번 패턴의 명중/회피 결과를 반영 (보스도 플레이어 패턴에 적응)
		if (UCombatPatternSubsystem* Subsystem = UGameplayStatics::GetGameInstance(this)->GetSubsystem<UCombatPatternSubsystem>())
		{
			Subsystem->RecordAttackResult(MonsterID, CurrentPatternIndex, fDamage > 0.f);
		}
	}

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
