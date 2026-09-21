#include "Mon_Narbash.h"
#include "Mon_NarbashAnim.h"
#include "Engine/DamageEvents.h"
#include "DrawDebugHelpers.h"
#include "MonsterAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/AudioComponent.h"
#include "UI_BossHpBar.h"
#include "CombatPatternSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Components/WidgetComponent.h"
#include "Py_Grey.h"
#include "StoryComponent.h"
#include "BossTelegraph.h"
#include "BossProjectile.h"
#include "BossCameraShake.h"
#include "BTTask_BossSkill.h"
#include "BTTask_BossApproach.h"
#include "BTDecorator_BossSkillReady.h"

#define BOSS_LOG(Format, ...) UE_LOG(LogTemp, Log, TEXT("[NarbashBoss] " Format), ##__VA_ARGS__)

namespace
{
	template<typename T>
	T* FindBossAsset(const TCHAR* Path)
	{
		ConstructorHelpers::FObjectFinder<T> Finder(Path);
		return Finder.Succeeded() ? Finder.Object : nullptr;
	}

	FString AnimPath(const TCHAR* Name)
	{
		return FString::Printf(TEXT("/Game/ParagonNarbash/Characters/Heroes/Narbash/Animations/%s.%s"), Name, Name);
	}

	FString FxPath(const TCHAR* SubPath, const TCHAR* Name)
	{
		return FString::Printf(TEXT("/Game/ParagonNarbash/FX/Particles/%s/%s.%s"), SubPath, Name, Name);
	}

	FString CuePath(const TCHAR* Name)
	{
		return FString::Printf(TEXT("/Game/ParagonNarbash/Audio/Cues/%s.%s"), Name, Name);
	}

	// 예고 표시 색상 (붉은색 계열로 통일)
	const FLinearColor TelegraphColor(1.f, 0.12f, 0.04f, 0.42f);
	const FLinearColor RingColor(1.f, 0.55f, 0.15f, 0.55f);

	// 콤보 단계별 피해 배율 (마지막 타격이 가장 아프다)
	const float ComboStepMultiplier[3] = { 1.f, 1.15f, 1.7f };
}

AMon_Narbash::AMon_Narbash()
{
	PrimaryActorTick.bCanEverTick = true;

	m_tInfo.iMaxHp = 1000;
	m_tInfo.iHp = 1000;
	m_tInfo.iAttack = 20;
	DisplayName = FText::FromString(TEXT("나르바쉬"));

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

	// 보스는 플레이어의 타격에 밀려나지 않는다
	KnockbackStrength = 0.f;

	// 밀려나지는 않지만 맞는 느낌은 나야 하므로 메시 반동만 작게 준다. 덩치가 커서 락온 마커도 키운다
	HitRecoilScale = 0.35f;
	LockOnMarkerScale = 1.8f;

	InitSkillConfigs();
	LoadBossAssets();
}

void AMon_Narbash::InitSkillConfigs()
{
	SkillConfigs.SetNum(static_cast<int32>(ENarbashSkill::Count));

	auto Set = [this](ENarbashSkill Skill, float MinRange, float MaxRange, float Cooldown, float Weight, int32 MinPhase, float Damage)
	{
		FNarbashSkillConfig& Cfg = SkillConfigs[static_cast<int32>(Skill)];
		Cfg.MinRange = MinRange;
		Cfg.MaxRange = MaxRange;
		Cfg.Cooldown = Cooldown;
		Cfg.Weight = Weight;
		Cfg.MinPhase = MinPhase;
		Cfg.DamageMultiplier = Damage;
	};

	//                              최소     최대    쿨다운  가중치  페이즈  피해배율
	Set(ENarbashSkill::Roar,        0.f,  100000.f,  0.f,   0.f,    1,     0.f);
	Set(ENarbashSkill::Combo,       0.f,    270.f,   0.f,   1.0f,   1,     1.f);
	Set(ENarbashSkill::Smash,       0.f,    360.f,   6.f,   0.8f,   1,     2.f);
	Set(ENarbashSkill::Charge,    550.f,   1700.f,   9.f,   0.85f,  1,     2.4f);
	Set(ENarbashSkill::LeapSlam,  450.f,   1300.f,  10.f,   0.8f,   1,     2.4f);
	Set(ENarbashSkill::DrumThrow, 500.f,   2200.f,   6.f,   0.55f,  1,     1.5f);
	Set(ENarbashSkill::Rampage,     0.f,   1400.f,  24.f,   1.3f,   2,     1.4f);
}

void AMon_Narbash::LoadBossAssets()
{
	Anim_Roar = FindBossAsset<UAnimSequence>(*AnimPath(TEXT("Cast")));
	Anim_Smash = FindBossAsset<UAnimSequence>(*AnimPath(TEXT("Primary_Swing3_Slow")));
	Anim_ChargeIntro = FindBossAsset<UAnimSequence>(*AnimPath(TEXT("Q_Ability_Intro")));
	Anim_ChargeRun = FindBossAsset<UAnimSequence>(*AnimPath(TEXT("Q_Ability_Sprint")));
	Anim_ChargeStop = FindBossAsset<UAnimSequence>(*AnimPath(TEXT("Q_Ability_Stop")));
	Anim_LeapStart = FindBossAsset<UAnimSequence>(*AnimPath(TEXT("Jump_Start")));
	Anim_LeapAir = FindBossAsset<UAnimSequence>(*AnimPath(TEXT("In_Air_Melee")));
	Anim_LeapLand = FindBossAsset<UAnimSequence>(*AnimPath(TEXT("Jump_Recovery")));
	Anim_Throw = FindBossAsset<UAnimSequence>(*AnimPath(TEXT("RMB_Ability_Throw")));
	Anim_RampageIntro = FindBossAsset<UAnimSequence>(*AnimPath(TEXT("R_Ability_Intro")));
	Anim_RampageLoop = FindBossAsset<UAnimSequence>(*AnimPath(TEXT("R_Ability")));
	Anim_RampageEnd = FindBossAsset<UAnimSequence>(*AnimPath(TEXT("R_Ability_End")));
	Anim_StunStart = FindBossAsset<UAnimSequence>(*AnimPath(TEXT("Stun_Start")));
	Anim_StunLoop = FindBossAsset<UAnimSequence>(*AnimPath(TEXT("Stun_Loop")));
	Anim_StunEnd = FindBossAsset<UAnimSequence>(*AnimPath(TEXT("Stun_End")));

	FX_MeleeImpact = FindBossAsset<UParticleSystem>(*FxPath(TEXT("Abilities/Primary/FX"), TEXT("P_Narbash_Melee_Impact")));
	FX_MeleeTrail = FindBossAsset<UParticleSystem>(*FxPath(TEXT("Abilities/Primary/FX"), TEXT("P_Narbash_MeleeAnimTrail")));
	FX_SeismicImpact = FindBossAsset<UParticleSystem>(*FxPath(TEXT("Abilities/Shield/FX"), TEXT("P_Narbash_SeismicImpact")));
	FX_BigKnockup = FindBossAsset<UParticleSystem>(*FxPath(TEXT("Abilities/Ultimate/FX"), TEXT("P_NarbashUlt_BigKnockup")));
	FX_EnergyBuildup = FindBossAsset<UParticleSystem>(*FxPath(TEXT("Abilities/Ultimate/FX"), TEXT("P_EnergyBuildup")));
	FX_DashCast = FindBossAsset<UParticleSystem>(*FxPath(TEXT("Abilities/Dash/FX"), TEXT("P_Narbash_Dash_Cast")));
	FX_DashGround = FindBossAsset<UParticleSystem>(*FxPath(TEXT("Abilities/Dash/FX"), TEXT("P_Narbash_Dash_GroundCast")));
	FX_UltLoop = FindBossAsset<UParticleSystem>(*FxPath(TEXT("Abilities/Ultimate/FX"), TEXT("P_Narbash_Ultimate_Looping")));
	FX_UltEnd = FindBossAsset<UParticleSystem>(*FxPath(TEXT("Abilities/Ultimate/FX"), TEXT("P_Narbash_Ultimate_End")));
	FX_MaceClacks = FindBossAsset<UParticleSystem>(*FxPath(TEXT("Abilities/Ultimate/FX"), TEXT("p_Narbash_MaceClacks")));
	FX_Aura = FindBossAsset<UParticleSystem>(*FxPath(TEXT("Abilities/Ultimate/FX"), TEXT("P_Narbash_Ultimate_Looping")));
	FX_AuraEnergy = FindBossAsset<UParticleSystem>(*FxPath(TEXT("Abilities/Ultimate/FX"), TEXT("P_Narbash_Ultimate_Drum_Looping")));

	SFX_Roar = FindBossAsset<USoundBase>(*CuePath(TEXT("Narbash_Emote_Taunt")));
	SFX_EffortHeavy = FindBossAsset<USoundBase>(*CuePath(TEXT("Narbash_Effort_Ability_Ultimate")));
	SFX_EffortSwing = FindBossAsset<USoundBase>(*CuePath(TEXT("Narbash_Effort_Swing")));
	SFX_EffortCharge = FindBossAsset<USoundBase>(*CuePath(TEXT("Narbash_Effort_Ability_Q")));
	SFX_Pain = FindBossAsset<USoundBase>(*CuePath(TEXT("Narbash_Effort_PainHeavy")));
	SFX_Impact = FindBossAsset<USoundBase>(*CuePath(TEXT("Narbash_Ability_Ultimate_Enemy")));
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

	// ---- 보스 브레인 초기화 ----
	SkillCooldowns.Init(0.f, static_cast<int32>(ENarbashSkill::Count));

	// 교전 초반부터 대형 스킬이 연달아 나오지 않도록 시작 쿨다운을 준다
	SkillCooldowns[static_cast<int32>(ENarbashSkill::Charge)] = 3.f;
	SkillCooldowns[static_cast<int32>(ENarbashSkill::LeapSlam)] = 5.f;
	SkillCooldowns[static_cast<int32>(ENarbashSkill::Smash)] = 2.f;
	SkillCooldowns[static_cast<int32>(ENarbashSkill::DrumThrow)] = 1.f;
	SkillCooldowns[static_cast<int32>(ENarbashSkill::Rampage)] = 6.f;

	Poise = MaxPoise;

	// 플레이어가 멀리 떨어져도 보스가 타겟을 놓치고 굳어버리지 않게 한다
	fTraceRange = FMath::Max(fTraceRange, MinBossTraceRange);

	// 코드 애니메이션(동적 몽타주)을 재생할 슬롯은 기존 콤보 몽타주가 쓰는 슬롯을 그대로 따른다
	if (IsValid(pNarbashAnim))
	{
		if (const UAnimMontage* ComboMontage = pNarbashAnim->GetComboMontage())
		{
			if (ComboMontage->SlotAnimTracks.Num() > 0)
				MontageSlotName = ComboMontage->SlotAnimTracks[0].SlotName;
		}
	}

	BOSS_LOG("BeginPlay slot=%s traceRange=%.0f anim=%s", *MontageSlotName.ToString(), fTraceRange, pNarbashAnim ? TEXT("ok") : TEXT("null"));
}

void AMon_Narbash::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(BossHpBarWidget))
	{
		BossHpBarWidget->RemoveFromParent();
		BossHpBarWidget = nullptr;
	}

	ClearSkillVisuals();
	RestoreChargeMovement();

	if (IsValid(AuraComponent))
		AuraComponent->DestroyComponent();

	if (IsValid(AuraEnergyComponent))
		AuraEnergyComponent->DestroyComponent();

	// 사망 슬로모션 도중 파괴/레벨 이동되어도 전역 시간이 느려진 채로 남지 않게 한다
	if (bTimeDilated)
		UGameplayStatics::SetGlobalTimeDilation(this, 1.f);

	Super::EndPlay(EndPlayReason);
}

// =========================================================================
// Tick
// =========================================================================
void AMon_Narbash::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDashing)
	{
		AMonsterAIController* pAIController = Cast<AMonsterAIController>(GetController());
		ACharacter* pTarget = IsValid(pAIController) ? pAIController->GetTarget() : nullptr;

		// 타겟을 잃었거나 근접 사거리에 도달하면 즉시 멈춘다 (안 그러면 8000 속도 돌진이 타겟을
		// 훌쩍 지나쳐서 탐지 범위 밖으로 나가버리고, 그러면 BT 전체가 타겟을 잃어 Idle로 굳어버린다)
		if (!IsValid(pTarget) || FVector::Dist2D(GetActorLocation(), pTarget->GetActorLocation()) <= GetAttackDist())
		{
			GetCharacterMovement()->StopMovementImmediately();
			DashSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			bIsDashing = false;
		}
	}

	if (bDead)
		return;

	// 강인도 회복: 일정 시간 맞지 않으면 서서히 차오른다
	TimeSinceLastHit += DeltaTime;
	if (!bStaggered && TimeSinceLastHit > PoiseRegenDelay)
		Poise = FMath::Min(MaxPoise, Poise + PoiseRegenPerSecond * DeltaTime);

	UpdateBrain(DeltaTime);

	if (bStaggered)
	{
		TickStagger(DeltaTime);
	}
	else if (ActiveSkill != ENarbashSkill::None)
	{
		SkillElapsed += DeltaTime;

		if (bTracking)
		{
			if (bTrackFixedDir)
				FaceDirection(TrackFixedDir, DeltaTime, TrackTurnRate);
			else
				FaceTarget(DeltaTime, TrackTurnRate);
		}

		TickSkillTimeline(DeltaTime);
		TickCharge(DeltaTime);
		TickLeap(DeltaTime);
	}

	TickShockRings(DeltaTime);
}

// =========================================================================
// 피격 / 페이즈 / 사망
// =========================================================================
void AMon_Narbash::Hit()
{
	// 보스는 피격 몽타주로 동작이 끊기지 않는다 (bHit을 켜지 않아 외곽 BT의 피격 브랜치도 타지 않는다)
	TriggerHitFlash();
}

float AMon_Narbash::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 페이즈 전환 연출 중에는 무적
	if (bPhaseTransitioning || bDead)
		return 0.f;

	// 스턴 중에는 약점이 노출되어 더 아프게 맞는다
	if (bStaggered)
		DamageAmount *= StaggerDamageMultiplier;

	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage > 0.f)
		TimeSinceLastHit = 0.f;

	if (m_tInfo.iHp <= 0)
	{
		if (IsValid(BossHpBarWidget))
			BossHpBarWidget->SetBossHp(0.f);

		OnBossDeath();
		return ActualDamage;
	}

	float HpRatio = GetHpRatio();

	if (IsValid(BossHpBarWidget))
		BossHpBarWidget->SetBossHp(HpRatio);

	// 강인도가 바닥나면 스턴. 포효/궁극기 같은 연출 스킬 중에는 끊기지 않는다
	const bool bPoiseImmune = ActiveSkill == ENarbashSkill::Roar || ActiveSkill == ENarbashSkill::Rampage;

	if (!bStaggered && !bPoiseImmune)
	{
		Poise -= ActualDamage;

		if (Poise <= 0.f)
			EnterStagger();
	}

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
	bPhaseRoarPending = true;

	if (Phase2AttackPatterns.Num() > 0)
	{
		AttackPatterns = Phase2AttackPatterns;
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed *= Phase2SpeedMultiplier;
	}

	// 진행 중이던 스킬/스턴을 끊고, 브레인이 다음 스킬로 포효(전환 연출)를 고르게 한다
	if (bStaggered)
		EndStagger();

	if (ActiveSkill != ENarbashSkill::None)
		CancelSkill();

	PendingSkill = ENarbashSkill::Roar;
	RecoveryRemaining = 0.f;
	Poise = MaxPoise;

	// 2페이즈에서는 쿨다운을 다시 짧게 시작해서 전환 직후부터 공세가 이어지게 한다
	SkillCooldowns[static_cast<int32>(ENarbashSkill::Rampage)] = 5.f;

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

	// 브레인이 포효 스킬을 못 돌리는 예외 상황(BT 정지 등)에서도 무적이 영원히 남지 않도록 안전장치
	GetWorldTimerManager().SetTimer(PhaseTransitionTimerHandle, this, &AMon_Narbash::EndPhaseTransition, PhaseTransitionInvincibleDuration, false);

	BOSS_LOG("2페이즈 돌입");
}

void AMon_Narbash::EndPhaseTransition()
{
	bPhaseTransitioning = false;
	bPhaseRoarPending = false;
}

void AMon_Narbash::OnBossDeath()
{
	if (bDead)
		return;

	bDead = true;

	if (ActiveSkill != ENarbashSkill::None || bChargeRunning)
	{
		// 사망 처리에서는 BT가 이미 정지되어 있으므로 종료 이벤트 없이 조용히 정리한다
		OnSkillFinished.Clear();
		ActiveSkill = ENarbashSkill::None;
	}

	SkillTimeline.Reset();
	ClearSkillVisuals();
	RestoreChargeMovement();
	EnableWeaponCollision(false);
	ShockRings.Empty();
	bChargeRunning = false;
	bLeapAirborne = false;
	bStaggered = false;
	bTracking = false;

	for (UParticleSystemComponent* Aura : { AuraComponent, AuraEnergyComponent })
	{
		if (IsValid(Aura))
		{
			Aura->DeactivateSystem();
			Aura->bAutoDestroy = true;
		}
	}

	// 죽는 순간의 임팩트: 폭발 이펙트 + 강한 카메라 셰이크 + 짧은 슬로모션
	const FVector Loc = GetActorLocation();
	SpawnFXAt(FX_BigKnockup, GetGroundLocation(Loc), FRotator::ZeroRotator, 1.6f);
	SpawnFXAt(FX_UltEnd, GetGroundLocation(Loc), FRotator::ZeroRotator, 1.4f);
	PlaySFX(SFX_Pain, Loc, 1.4f);
	ShakePlayerCamera(UBossShake_Roar::StaticClass(), 1.2f);

	UGameplayStatics::SetGlobalTimeDilation(this, 0.25f);
	bTimeDilated = true;

	GetWorldTimerManager().SetTimer(DeathSlowmoTimerHandle, [this]()
	{
		UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
		bTimeDilated = false;
	}, 0.4f, false);

	BOSS_LOG("사망");
}

// =========================================================================
// 무기 콜리전 (콤보)
// =========================================================================
void AMon_Narbash::LaunchCharacter(FVector LaunchVelocity, bool bXYOverride, bool bZOverride)
{
	if (bInternalLaunch)
		Super::LaunchCharacter(LaunchVelocity, bXYOverride, bZOverride);
}

void AMon_Narbash::InternalLaunch(const FVector& Velocity, bool bXYOverride, bool bZOverride)
{
	bInternalLaunch = true;
	LaunchCharacter(Velocity, bXYOverride, bZOverride);
	bInternalLaunch = false;
}

void AMon_Narbash::EnableWeaponCollision(bool bEnable)
{
	// 켤 때/끌 때 모두 초기화해서 다음 스윙(다음 콤보 포함)에서 다시 한 번 맞을 수 있게 한다
	HitActorsThisSwing.Empty();

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
	bIsDashing = true;

	FVector DashDirection = GetActorForwardVector();
	InternalLaunch(DashDirection * 8000.f, true, true);
}

void AMon_Narbash::OnCollisionOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	FDamageEvent	DamageEvent;
	OtherActor->TakeDamage(m_tInfo.iAttack * 3.f, DamageEvent, GetController(), this);
}

void AMon_Narbash::WepaonBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsValid(OtherActor) || OtherActor == this || OtherActor->IsA<AMonster>() || HitActorsThisSwing.Contains(OtherActor))
		return;

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
		HitActorsThisSwing.Add(OtherActor);

		// 콤보 단계가 올라갈수록 세게 때린다
		const int32 ComboIndex = IsValid(pNarbashAnim) ? FMath::Clamp(pNarbashAnim->GetCurrentCombo() - 1, 0, 2) : 0;
		const float Multiplier = GetConfig(ENarbashSkill::Combo).DamageMultiplier * ComboStepMultiplier[ComboIndex];

		float fDamage = OtherActor->TakeDamage(m_tInfo.iAttack * Multiplier, DamageEvent, GetController(), this);

		// 7단계 적응형 AI 학습에 이번 패턴의 명중/회피 결과를 반영 (보스도 플레이어 패턴에 적응)
		RecordSkillResult(fDamage > 0.f);

		if (fDamage > 0.f)
		{
			bSkillConnected = true;

			const FVector HitLoc = vImpactPoint.IsNearlyZero() ? OtherActor->GetActorLocation() : vImpactPoint;
			SpawnFXAt(FX_MeleeImpact, HitLoc, FRotator::ZeroRotator, 1.3f);

			FVector KnockDir = OtherActor->GetActorLocation() - GetActorLocation();
			KnockDir.Z = 0.f;
			KnockDir = KnockDir.GetSafeNormal();

			if (ACharacter* Victim = Cast<ACharacter>(OtherActor))
			{
				const float Strength = ComboIndex == 2 ? 750.f : 320.f;
				Victim->LaunchCharacter(KnockDir * Strength + FVector(0.f, 0.f, ComboIndex == 2 ? 260.f : 90.f), true, true);
			}

			ShakePlayerCamera(ComboIndex == 2 ? UBossShake_Heavy::StaticClass() : UBossShake_Light::StaticClass(), 1.f);
			PlaySFX(SFX_Impact, HitLoc, 0.6f);
		}
	}

#if ENABLE_DRAW_DEBUG
	FVector vCenter = (vStart + vEnd) / 2.f;
	FColor	DrawColor = HitArray.Num() > 0 ? FColor::Yellow : FColor::Black;

	DrawDebugBox(GetWorld(), vCenter, FVector(15.0f, 50.f, 15.f), vDir.ToOrientationQuat(), DrawColor, false, 1.5f);

#endif
}

void AMon_Narbash::WeaponEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}

// 콤보 몽타주 노티파이 훅 ---------------------------------------------------
void AMon_Narbash::OnComboSwingBegin()
{
	if (ActiveSkill != ENarbashSkill::Combo)
		return;

	// 휘두르는 동안은 방향을 고정해서 옆으로 피할 수 있게 한다
	bTracking = false;

	// 스윙마다 앞으로 파고든다: 멀리 있으면 크게, 이미 붙어 있으면 거의 안 움직인다
	if (ACharacter* Target = GetTargetChar())
	{
		const float Dist = GetDistanceToTarget2D();
		const float Lunge = FMath::Clamp((Dist - 110.f) * 3.2f, 0.f, 620.f);

		if (Lunge > 30.f)
			InternalLaunch(GetActorForwardVector() * Lunge, true, false);
	}

	PlaySFX(SFX_EffortSwing, GetActorLocation(), 0.8f);
	ShakePlayerCamera(UBossShake_Light::StaticClass(), 0.35f, 1500.f);
}

void AMon_Narbash::OnComboSwingWindowEnd()
{
	// 다음 스윙을 준비하는 동안에는 다시 플레이어를 향해 돌아선다
	if (ActiveSkill == ENarbashSkill::Combo)
	{
		bTracking = true;
		bTrackFixedDir = false;
		TrackTurnRate = 420.f;
	}
}

float AMon_Narbash::GetComboChainChance() const
{
	return CurrentPhase >= 2 ? 0.88f : 0.62f;
}

void AMon_Narbash::OnComboMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// 우리가 직접 끊은(CancelSkill/스턴) 경우에는 이미 정리가 끝난 상태다
	if (bInterrupted)
		return;

	if (ActiveSkill == ENarbashSkill::Combo)
		EndSkill(true);
}

void AMon_Narbash::NotifyProjectileResult(bool bDidHit)
{
	if (bDead)
		return;

	if (UCombatPatternSubsystem* Subsystem = UGameplayStatics::GetGameInstance(this)->GetSubsystem<UCombatPatternSubsystem>())
		Subsystem->RecordAttackResult(MonsterID, static_cast<int32>(ENarbashSkill::DrumThrow), bDidHit);
}

// =========================================================================
// 브레인
// =========================================================================
const FNarbashSkillConfig& AMon_Narbash::GetConfig(ENarbashSkill Skill) const
{
	static const FNarbashSkillConfig Fallback;
	const int32 Index = static_cast<int32>(Skill);
	return SkillConfigs.IsValidIndex(Index) ? SkillConfigs[Index] : Fallback;
}

ACharacter* AMon_Narbash::GetTargetChar() const
{
	if (AMonsterAIController* AICon = Cast<AMonsterAIController>(GetController()))
	{
		if (AICon->GetBlackboardComponent())
			return AICon->GetTarget();
	}

	return nullptr;
}

float AMon_Narbash::GetDistanceToTarget2D() const
{
	if (const ACharacter* Target = GetTargetChar())
		return FVector::Dist2D(GetActorLocation(), Target->GetActorLocation());

	return 0.f;
}

bool AMon_Narbash::IsSkillOffCooldown(ENarbashSkill Skill) const
{
	const int32 Index = static_cast<int32>(Skill);
	return !SkillCooldowns.IsValidIndex(Index) || SkillCooldowns[Index] <= 0.f;
}

void AMon_Narbash::UpdateBrain(float DeltaTime)
{
	for (float& Cooldown : SkillCooldowns)
		Cooldown = FMath::Max(0.f, Cooldown - DeltaTime);

	RecoveryRemaining = FMath::Max(0.f, RecoveryRemaining - DeltaTime);

	if (ActiveSkill != ENarbashSkill::None || bStaggered)
		return;

	if (!IsValid(GetTargetChar()))
	{
		PendingSkill = ENarbashSkill::None;
		return;
	}

	PendingAge += DeltaTime;

	bool bRepick = PendingSkill == ENarbashSkill::None || PendingAge > 3.5f;

	if (!bRepick && PendingSkill != ENarbashSkill::Roar)
	{
		const FNarbashSkillConfig& Cfg = GetConfig(PendingSkill);
		const float Dist = GetDistanceToTarget2D();

		// 골라 둔 스킬이 쿨다운 중이거나, 너무 가까워져서 못 쓰게 됐으면 다시 고른다
		if (!IsSkillOffCooldown(PendingSkill) || (PendingAge > 0.3f && Dist < Cfg.MinRange - 40.f))
			bRepick = true;
	}

	// 조우 포효/페이즈 전환 포효는 다른 판단보다 항상 우선한다
	if ((!bEngaged || bPhaseRoarPending) && PendingSkill != ENarbashSkill::Roar)
		bRepick = true;

	if (bRepick)
		PickNextSkill();
}

ENarbashSkill AMon_Narbash::PickNextSkill()
{
	PendingAge = 0.f;

	if (!bEngaged || bPhaseRoarPending)
	{
		PendingSkill = ENarbashSkill::Roar;
		return PendingSkill;
	}

	const float Dist = GetDistanceToTarget2D();

	UCombatPatternSubsystem* Subsystem = nullptr;
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
		Subsystem = GI->GetSubsystem<UCombatPatternSubsystem>();

	const float AvgEngageDist = Subsystem ? Subsystem->GetAvgEngageDistance() : 0.f;
	const float DodgeRate = Subsystem ? Subsystem->GetDodgeRate() : 0.f;

	// 사거리 밖이어도 이 정도 안쪽이면 "다가가서 쓴다"는 선택지로 남긴다
	constexpr float ApproachReach = 700.f;

	struct FCandidate
	{
		ENarbashSkill Skill;
		float Weight;
	};

	TArray<FCandidate> Candidates;
	float TotalWeight = 0.f;

	for (int32 i = static_cast<int32>(ENarbashSkill::Combo); i < static_cast<int32>(ENarbashSkill::Count); ++i)
	{
		const ENarbashSkill Skill = static_cast<ENarbashSkill>(i);
		const FNarbashSkillConfig& Cfg = GetConfig(Skill);

		if (CurrentPhase < Cfg.MinPhase || !IsSkillOffCooldown(Skill))
			continue;

		float Weight = Cfg.Weight;

		if (Dist > Cfg.MaxRange)
		{
			if (Dist - Cfg.MaxRange > ApproachReach)
				continue;

			Weight *= 0.4f;
		}
		else if (Dist < Cfg.MinRange)
		{
			if (Cfg.MinRange - Dist > 300.f)
				continue;

			Weight *= 0.15f;
		}

		// 같은 스킬이 연달아 나오면 단조로우니 강하게 감점
		if (Skill == LastSkill)
			Weight *= 0.3f;
		else if (Skill == LastLastSkill)
			Weight *= 0.65f;

		// 7단계 적응형 AI: 지금까지 잘 맞은 스킬일수록 가중치 상향 (0.6~1.4배)
		if (Subsystem)
			Weight *= 0.6f + 0.8f * Subsystem->GetPatternWeight(MonsterID, i);

		// 멀리서 견제만 하는 플레이어에게는 원거리/돌격 스킬을, 회피를 잘 하는 플레이어에게는 광역기를
		const bool bRanged = Skill == ENarbashSkill::Charge || Skill == ENarbashSkill::LeapSlam || Skill == ENarbashSkill::DrumThrow;
		if (AvgEngageDist > 650.f && bRanged)
			Weight *= 1.5f;

		if (DodgeRate > 0.6f && (Skill == ENarbashSkill::Smash || Skill == ENarbashSkill::Rampage))
			Weight *= 1.3f;

		if (Weight > 0.f)
		{
			Candidates.Add({ Skill, Weight });
			TotalWeight += Weight;
		}
	}

	if (Candidates.Num() == 0)
	{
		PendingSkill = ENarbashSkill::Combo;
		return PendingSkill;
	}

	float Roll = FMath::FRandRange(0.f, TotalWeight);

	for (const FCandidate& Candidate : Candidates)
	{
		Roll -= Candidate.Weight;

		if (Roll <= 0.f)
		{
			PendingSkill = Candidate.Skill;
			return PendingSkill;
		}
	}

	PendingSkill = Candidates.Last().Skill;
	return PendingSkill;
}

bool AMon_Narbash::IsPendingSkillReady() const
{
	if (PendingSkill == ENarbashSkill::None || IsBusy() || bDead || RecoveryRemaining > 0.f)
		return false;

	if (!IsValid(GetTargetChar()))
		return false;

	// 조우 포효는 플레이어가 어느 정도 가까이 왔을 때 터뜨려야 연출이 산다. 페이즈 전환 포효는 거리와 무관
	if (PendingSkill == ENarbashSkill::Roar)
		return bPhaseRoarPending || GetDistanceToTarget2D() <= IntroRoarDistance;

	if (!IsSkillOffCooldown(PendingSkill))
		return false;

	const FNarbashSkillConfig& Cfg = GetConfig(PendingSkill);
	const float Dist = GetDistanceToTarget2D();

	return Dist >= Cfg.MinRange && Dist <= Cfg.MaxRange;
}

float AMon_Narbash::GetPendingApproachDistance() const
{
	if (PendingSkill == ENarbashSkill::None)
		return 0.f;

	// 조우 포효는 사거리 안쪽까지만 다가간다 (페이즈 전환 포효는 제자리에서 바로 시전되므로 이 값이 쓰이지 않는다)
	if (PendingSkill == ENarbashSkill::Roar)
		return IntroRoarDistance * 0.85f;

	// 최대 사거리의 80%까지 접근하면 여유 있게 시전할 수 있다
	return GetConfig(PendingSkill).MaxRange * 0.8f;
}

bool AMon_Narbash::BeginPendingSkill()
{
	if (!IsPendingSkillReady())
		return false;

	return BeginSkill(PendingSkill);
}

void AMon_Narbash::CancelSkill()
{
	if (ActiveSkill == ENarbashSkill::None)
		return;

	if (IsValid(pNarbashAnim))
		pNarbashAnim->StopAllMontages(0.15f);

	EnableWeaponCollision(false);
	EndSkill(false);
}

void AMon_Narbash::FaceTarget(float DeltaTime, float TurnRate)
{
	const ACharacter* Target = GetTargetChar();

	if (!IsValid(Target))
		return;

	FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;

	FaceDirection(ToTarget, DeltaTime, TurnRate);
}

void AMon_Narbash::FaceDirection(const FVector& Direction, float DeltaTime, float TurnRate)
{
	if (Direction.IsNearlyZero())
		return;

	FRotator Current = GetActorRotation();
	const float TargetYaw = Direction.Rotation().Yaw;
	const float NewYaw = FMath::FixedTurn(Current.Yaw, TargetYaw, TurnRate * DeltaTime);

	SetActorRotation(FRotator(0.f, NewYaw, 0.f));
}

// =========================================================================
// 스킬 공통
// =========================================================================
void AMon_Narbash::At(float Time, TFunction<void()> Action)
{
	FSkillEvent Event;
	Event.Time = Time;
	Event.Action = MoveTemp(Action);
	SkillTimeline.Add(MoveTemp(Event));
}

void AMon_Narbash::TickSkillTimeline(float DeltaTime)
{
	// 이벤트 실행 중에 새 이벤트가 추가되거나 스킬이 끝나서 배열이 비워질 수 있으므로 인덱스로 순회하고
	// 매번 유효성을 다시 확인한다
	const int32 SkillToken = SkillSerial;

	for (int32 i = 0; i < SkillTimeline.Num(); ++i)
	{
		if (SkillToken != SkillSerial || ActiveSkill == ENarbashSkill::None)
			return;

		if (SkillTimeline[i].bFired || SkillTimeline[i].Time > SkillElapsed)
			continue;

		SkillTimeline[i].bFired = true;
		TFunction<void()> Action = SkillTimeline[i].Action;

		if (Action)
			Action();
	}
}

bool AMon_Narbash::BeginSkill(ENarbashSkill Skill)
{
	if (IsBusy() || bDead || !IsValid(pNarbashAnim))
		return false;

	ActiveSkill = Skill;
	PendingSkill = ENarbashSkill::None;
	++SkillSerial;
	SkillElapsed = 0.f;
	SkillTimeline.Reset();
	bSkillConnected = false;
	CurrentPatternIndex = static_cast<int32>(Skill);
	bArmored = true;
	bTracking = false;
	bTrackFixedDir = false;
	HitActorsThisSwing.Empty();

	GetCharacterMovement()->StopMovementImmediately();

	if (AAIController* AICon = Cast<AAIController>(GetController()))
		AICon->StopMovement();

	BOSS_LOG("스킬 시작: %d (거리 %.0f, 페이즈 %d)", static_cast<int32>(Skill), GetDistanceToTarget2D(), CurrentPhase);

	switch (Skill)
	{
	case ENarbashSkill::Roar:		StartRoar(bPhaseRoarPending); break;
	case ENarbashSkill::Combo:		StartCombo(); break;
	case ENarbashSkill::Smash:		StartSmash(); break;
	case ENarbashSkill::Charge:		StartCharge(); break;
	case ENarbashSkill::LeapSlam:	StartLeapSlam(); break;
	case ENarbashSkill::DrumThrow:	StartDrumThrow(); break;
	case ENarbashSkill::Rampage:	StartRampage(); break;
	default:
		EndSkill(false);
		return false;
	}

	// 시간 0에 등록된 이벤트(즉시 시작 동작)는 바로 실행한다
	if (ActiveSkill == Skill)
		TickSkillTimeline(0.f);

	return true;
}

void AMon_Narbash::EndSkill(bool bCompleted)
{
	if (ActiveSkill == ENarbashSkill::None)
		return;

	const ENarbashSkill Finished = ActiveSkill;

	BOSS_LOG("스킬 종료: %d (%s, 명중=%s)", static_cast<int32>(Finished), bCompleted ? TEXT("완료") : TEXT("중단"), bSkillConnected ? TEXT("예") : TEXT("아니오"));

	// 명중하지 못한 스킬은 "회피당함"으로 학습에 반영한다. 투척은 투사체가 직접 결과를 보고한다
	if (bCompleted && !bSkillConnected && Finished != ENarbashSkill::Roar && Finished != ENarbashSkill::DrumThrow)
		RecordSkillResult(false);

	++SkillSerial;
	SkillTimeline.Reset();
	ClearSkillVisuals();
	RestoreChargeMovement();
	bChargeRunning = false;
	bLeapAirborne = false;
	bTracking = false;
	bTrackFixedDir = false;
	bArmored = false;
	EnableWeaponCollision(false);

	const int32 Index = static_cast<int32>(Finished);
	if (SkillCooldowns.IsValidIndex(Index))
	{
		const float Scale = CurrentPhase >= 2 ? Phase2CooldownScale : 1.f;
		SkillCooldowns[Index] = GetConfig(Finished).Cooldown * Scale;
	}

	if (Finished != ENarbashSkill::Roar)
	{
		LastLastSkill = LastSkill;
		LastSkill = Finished;
	}

	if (bCompleted)
	{
		// 후딜: 이 시간 동안 플레이어가 반격/재정비할 틈이 생긴다. 2페이즈는 훨씬 짧다
		const float Recovery = FMath::FRandRange(RecoveryTimeRange.X, RecoveryTimeRange.Y);
		RecoveryRemaining = Finished == ENarbashSkill::Roar ? 0.3f : Recovery * (CurrentPhase >= 2 ? Phase2RecoveryScale : 1.f);
	}

	ActiveSkill = ENarbashSkill::None;

	OnSkillFinished.Broadcast(bCompleted);
}

void AMon_Narbash::RecordSkillResult(bool bDidHit)
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		if (UCombatPatternSubsystem* Subsystem = GI->GetSubsystem<UCombatPatternSubsystem>())
			Subsystem->RecordAttackResult(MonsterID, CurrentPatternIndex, bDidHit);
	}
}

// =========================================================================
// 스킬 구현
// =========================================================================
void AMon_Narbash::StartRoar(bool bPhaseTransition)
{
	bEngaged = true;
	bArmored = true;
	bTracking = !bPhaseTransition;
	bTrackFixedDir = false;
	TrackTurnRate = 180.f;

	// 양손을 번쩍 드는 동작(Cast)을 느리게 재생해 포효처럼 보이게 한다. 팔이 정점에 이르는 시점에 충격파가 터진다
	const float AnimLength = PlaySeq(Anim_Roar, 0.6f, 0.2f, 0.25f);
	const float Duration = FMath::Max(AnimLength, bPhaseTransition ? 2.6f : 2.0f);
	const float BurstTime = AnimLength * 0.42f;

	SpawnFXAttached(FX_EnergyBuildup, TEXT("Chest"), bPhaseTransition ? 1.6f : 1.2f);
	PlaySFX(SFX_Roar, GetActorLocation(), 1.4f);

	At(BurstTime, [this, bPhaseTransition]()
	{
		bTracking = false;

		const FVector Ground = GetGroundLocation(GetActorLocation());
		SpawnFXAt(FX_BigKnockup, Ground, FRotator::ZeroRotator, bPhaseTransition ? 1.5f : 1.f);
		SpawnFXAt(FX_UltEnd, Ground, FRotator::ZeroRotator, bPhaseTransition ? 1.3f : 0.9f);
		SpawnShockRing(GetActorLocation(), bPhaseTransition ? 1100.f : 800.f, 0.8f, 0.f);

		PushPlayerAway(bPhaseTransition ? 900.f : 700.f, bPhaseTransition ? 1100.f : 700.f, 240.f);
		ShakePlayerCamera(UBossShake_Roar::StaticClass(), bPhaseTransition ? 1.3f : 1.f);
		PlaySFX(SFX_EffortHeavy, GetActorLocation(), 1.2f);

		if (bPhaseTransition)
			ActivateEnrageAura();
	});

	At(Duration, [this, bPhaseTransition]()
	{
		if (bPhaseTransition)
			EndPhaseTransition();

		EndSkill(true);
	});
}

void AMon_Narbash::StartCombo()
{
	UAnimMontage* ComboMontage = pNarbashAnim->GetComboMontage();

	if (!IsValid(ComboMontage))
	{
		EndSkill(false);
		return;
	}

	bTracking = true;
	bTrackFixedDir = false;
	TrackTurnRate = 380.f;

	const float FeintDelay = RollFeintDelay();

	At(FeintDelay, [this, ComboMontage]()
	{
		// 기존 몽타주가 느린(Slow) 스윙이라 보스답게 조금 빠르게 휘두르게 한다
		const float PlayTime = pNarbashAnim->Montage_Play(ComboMontage, GetSkillSpeed() * ComboBaseRate);

		if (PlayTime <= 0.f)
		{
			EndSkill(false);
			return;
		}

		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AMon_Narbash::OnComboMontageEnded);
		pNarbashAnim->Montage_SetEndDelegate(EndDelegate, ComboMontage);

		PlaySFX(SFX_EffortSwing, GetActorLocation(), 0.8f);
	});
}

void AMon_Narbash::StartSmash()
{
	bTracking = true;
	bTrackFixedDir = false;
	TrackTurnRate = 520.f;

	const float FeintDelay = RollFeintDelay();
	const float Rate = 0.9f * GetSkillSpeed();

	At(FeintDelay, [this, Rate, FeintDelay]()
	{
		const float Length = PlaySeq(Anim_Smash, Rate);
		const float StrikeTime = FMath::Max(Length * SmashStrikeFraction, 0.5f);
		const float LockTime = 0.22f;

		PlaySFX(SFX_EffortHeavy, GetActorLocation(), 0.9f);

		// 잠깐 플레이어 쪽으로 돌아선 뒤 방향을 잠그고, 그 자리에 예고 원을 띄운다
		At(SkillElapsed + LockTime, [this, StrikeTime, LockTime, Length]()
		{
			bTracking = false;

			const FVector Center = GetActorLocation() + GetActorForwardVector() * SmashForwardOffset;
			TrackTelegraph(ABossTelegraph::SpawnCircle(GetWorld(), Center, SmashRadius, StrikeTime - LockTime, TelegraphColor));

			At(SkillElapsed + (StrikeTime - LockTime), [this, Center]()
			{
				const FVector Ground = GetGroundLocation(Center);

				SpawnFXAt(FX_SeismicImpact, Ground, FRotator::ZeroRotator, 1.4f);
				SpawnFXAt(FX_MaceClacks, Ground, FRotator::ZeroRotator, 1.f);
				ShakePlayerCamera(UBossShake_Heavy::StaticClass(), 1.f);
				PlaySFX(SFX_Impact, Center, 1.f);

				if (IsTargetInCircle(Center, SmashRadius))
				{
					FVector Away = GetTargetChar()->GetActorLocation() - Center;
					Away.Z = 0.f;
					DamagePlayer(GetConfig(ENarbashSkill::Smash).DamageMultiplier, Away.GetSafeNormal(), 800.f, 380.f);
				}

				// 2페이즈: 충격파가 한 번 더 퍼져나간다 (점프/대시로 회피)
				if (CurrentPhase >= 2)
					SpawnShockRing(Center, 620.f, 0.6f, 1.f);
			});

			At(SkillElapsed + (Length - LockTime) + 0.05f, [this]() { EndSkill(true); });
		});
	});
}

void AMon_Narbash::StartCharge()
{
	bTracking = true;
	bTrackFixedDir = false;
	TrackTurnRate = 540.f;

	BeginChargeWindup(false);
}

void AMon_Narbash::BeginChargeWindup(bool bSecond)
{
	bChargeSecondRun = bSecond;
	bChargeHitThisRun = false;

	ACharacter* Target = GetTargetChar();
	FVector Dir = IsValid(Target) ? Target->GetActorLocation() - GetActorLocation() : GetActorForwardVector();
	Dir.Z = 0.f;
	Dir = Dir.GetSafeNormal();

	// 움직이는 플레이어를 약간 앞서 노린다 (완전 정확하진 않아서 옆으로 빠지면 피할 수 있다)
	if (IsValid(Target))
	{
		const FVector Lead = Target->GetVelocity() * 0.35f;
		FVector LeadDir = (Target->GetActorLocation() + Lead) - GetActorLocation();
		LeadDir.Z = 0.f;
		Dir = LeadDir.GetSafeNormal();
	}

	ChargeDir = Dir;
	ChargeMaxDistance = FMath::Clamp(GetDistanceToTarget2D() + 350.f, 900.f, 2000.f);

	const float Windup = (bSecond ? 0.55f : ChargeWindupTime) / GetSkillSpeed();

	bTracking = true;
	bTrackFixedDir = true;
	TrackFixedDir = ChargeDir;
	TrackTurnRate = 620.f;

	TrackTelegraph(ABossTelegraph::SpawnLine(GetWorld(), GetActorLocation(), ChargeDir, ChargeMaxDistance, 230.f, Windup, TelegraphColor));

	const float IntroLen = PlaySeq(Anim_ChargeIntro, 1.f, 0.15f, 0.1f);
	if (IntroLen > 0.f)
	{
		// 예고 시간에 맞춰 도입 동작 속도를 조정 (너무 느리거나 빠르지 않게 제한)
		ActiveSkillMontageRateHint = FMath::Clamp(IntroLen / Windup, 0.7f, 2.2f);
		if (IsValid(pNarbashAnim) && IsValid(CurrentDynamicMontage))
			pNarbashAnim->Montage_SetPlayRate(CurrentDynamicMontage, ActiveSkillMontageRateHint);
	}

	SpawnFXAttached(FX_EnergyBuildup, TEXT("Chest"), 1.f);
	PlaySFX(SFX_EffortCharge, GetActorLocation(), 1.f);

	At(SkillElapsed + Windup, [this]() { BeginChargeRun(); });
}

void AMon_Narbash::BeginChargeRun()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();

	SavedMaxWalkSpeed = Movement->MaxWalkSpeed;
	SavedMaxAcceleration = Movement->MaxAcceleration;
	bSavedOrientToMovement = Movement->bOrientRotationToMovement;
	bChargeMovementOverridden = true;

	Movement->MaxWalkSpeed = ChargeSpeed;
	Movement->MaxAcceleration = 30000.f;
	Movement->bOrientRotationToMovement = false;

	bChargeRunning = true;
	bTracking = false;
	bTrackFixedDir = false;
	ChargeTraveled = 0.f;
	ChargeLastLocation = GetActorLocation();
	ChargeRunElapsed = 0.f;
	ChargeBlockedTime = 0.f;

	PlaySeq(Anim_ChargeRun, 1.f, 0.1f, 0.1f, 999);

	SpawnFXAttached(FX_DashCast, NAME_None, 1.2f);
	SpawnFXAt(FX_DashGround, GetGroundLocation(GetActorLocation()), ChargeDir.Rotation(), 1.2f);
	ShakePlayerCamera(UBossShake_Light::StaticClass(), 0.8f);
}

void AMon_Narbash::TickCharge(float DeltaTime)
{
	if (!bChargeRunning)
		return;

	ChargeRunElapsed += DeltaTime;

	SetActorRotation(FRotator(0.f, ChargeDir.Rotation().Yaw, 0.f));
	AddMovementInput(ChargeDir, 1.f);

	const FVector Loc = GetActorLocation();
	const float Step = FVector::Dist2D(Loc, ChargeLastLocation);
	ChargeTraveled += Step;
	ChargeLastLocation = Loc;

	// 벽에 막혀서 거의 못 움직이면 돌진 종료
	if (ChargeRunElapsed > 0.3f)
	{
		if (Step < ChargeSpeed * DeltaTime * 0.35f)
			ChargeBlockedTime += DeltaTime;
		else
			ChargeBlockedTime = 0.f;
	}

	if (ACharacter* Target = GetTargetChar())
	{
		if (!bChargeHitThisRun && FVector::Dist2D(Loc, Target->GetActorLocation()) <= ChargeHitRadius)
		{
			bChargeHitThisRun = true;
			const float Dealt = DamagePlayer(GetConfig(ENarbashSkill::Charge).DamageMultiplier, ChargeDir, 1000.f, 380.f);

			if (Dealt > 0.f)
			{
				SpawnFXAt(FX_MeleeImpact, Target->GetActorLocation(), FRotator::ZeroRotator, 1.8f);
				ShakePlayerCamera(UBossShake_Heavy::StaticClass(), 1.f);
				PlaySFX(SFX_Impact, Loc, 1.f);
				EndChargeRun(true);
				return;
			}
		}
	}

	if (ChargeTraveled >= ChargeMaxDistance || ChargeBlockedTime > 0.12f)
		EndChargeRun(false);
}

void AMon_Narbash::EndChargeRun(bool bHitPlayer)
{
	bChargeRunning = false;
	RestoreChargeMovement();

	GetCharacterMovement()->Velocity *= 0.2f;

	// 벽에 부딪혀 멈춘 경우 충돌 연출
	if (!bHitPlayer && ChargeBlockedTime > 0.12f)
	{
		SpawnFXAt(FX_SeismicImpact, GetGroundLocation(GetActorLocation() + ChargeDir * 100.f), FRotator::ZeroRotator, 1.2f);
		ShakePlayerCamera(UBossShake_Heavy::StaticClass(), 0.7f);
	}

	// 2페이즈: 멈추지 않고 곧바로 방향을 다시 잡아 한 번 더 돌진한다
	if (CurrentPhase >= 2 && !bChargeSecondRun && FMath::FRand() < 0.6f)
	{
		BeginChargeWindup(true);
		return;
	}

	const float StopLen = PlaySeq(Anim_ChargeStop, 1.f, 0.1f, 0.15f);
	At(SkillElapsed + FMath::Max(StopLen, 0.3f), [this]() { EndSkill(true); });
}

void AMon_Narbash::RestoreChargeMovement()
{
	if (!bChargeMovementOverridden)
		return;

	bChargeMovementOverridden = false;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = SavedMaxWalkSpeed;
		Movement->MaxAcceleration = SavedMaxAcceleration;
		Movement->bOrientRotationToMovement = bSavedOrientToMovement;
	}
}

void AMon_Narbash::StartLeapSlam()
{
	bTracking = true;
	bTrackFixedDir = false;
	TrackTurnRate = 520.f;

	const float Windup = LeapWindupTime / GetSkillSpeed();

	PlaySeq(Anim_LeapStart, 1.f, 0.15f, 0.1f);
	PlaySFX(SFX_EffortCharge, GetActorLocation(), 1.f);
	SpawnFXAttached(FX_EnergyBuildup, TEXT("Chest"), 0.9f);

	// 도약 직전에 착지 지점을 확정하고 예고 원을 띄운다 (이후 플레이어가 움직이면 피할 수 있다)
	At(0.2f, [this, Windup]()
	{
		bTracking = false;

		ACharacter* Target = GetTargetChar();
		FVector Goal = IsValid(Target) ? Target->GetActorLocation() : GetActorLocation() + GetActorForwardVector() * 600.f;

		if (IsValid(Target))
			Goal += Target->GetVelocity() * 0.3f;

		// 너무 멀거나 가까우면 도약 거리를 제한한다
		FVector ToGoal = Goal - GetActorLocation();
		ToGoal.Z = 0.f;
		const float Dist = FMath::Clamp(ToGoal.Size(), 250.f, 1400.f);
		Goal = GetActorLocation() + ToGoal.GetSafeNormal() * Dist;

		LeapGoal = GetGroundLocation(Goal);

		const float FillTime = (Windup - 0.2f) + LeapAirTime;
		TrackTelegraph(ABossTelegraph::SpawnCircle(GetWorld(), LeapGoal, LeapRadius, FillTime, TelegraphColor));
	});

	At(Windup, [this]() { LaunchLeap(); });
}

void AMon_Narbash::LaunchLeap()
{
	FVector Delta = LeapGoal - GetActorLocation();
	Delta.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	const float T = LeapAirTime;
	const float Gravity = -GetWorld()->GetGravityZ() * GetCharacterMovement()->GravityScale;

	const FVector Velocity(Delta.X / T, Delta.Y / T, (Delta.Z + 0.5f * Gravity * T * T) / T);

	SetActorRotation(FRotator(0.f, FVector(Delta.X, Delta.Y, 0.f).Rotation().Yaw, 0.f));
	InternalLaunch(Velocity, true, true);

	bLeapAirborne = true;
	LeapAirElapsed = 0.f;

	const float AirLen = PlaySeq(Anim_LeapAir, 1.f, 0.1f, 0.1f);
	if (AirLen > 0.f && IsValid(CurrentDynamicMontage))
		pNarbashAnim->Montage_SetPlayRate(CurrentDynamicMontage, FMath::Clamp(AirLen / LeapAirTime, 0.6f, 2.f));

	SpawnFXAt(FX_DashGround, GetGroundLocation(GetActorLocation()), FRotator::ZeroRotator, 1.3f);
	PlaySFX(SFX_EffortHeavy, GetActorLocation(), 1.f);
}

void AMon_Narbash::TickLeap(float DeltaTime)
{
	if (!bLeapAirborne)
		return;

	LeapAirElapsed += DeltaTime;

	const bool bLanded = LeapAirElapsed > 0.25f && GetCharacterMovement()->IsMovingOnGround();

	if (bLanded || LeapAirElapsed > LeapAirTime + 1.2f)
		OnLeapLanded();
}

void AMon_Narbash::OnLeapLanded()
{
	bLeapAirborne = false;
	GetCharacterMovement()->StopMovementImmediately();

	const FVector Center = GetActorLocation();
	const FVector Ground = GetGroundLocation(Center);

	SpawnFXAt(FX_SeismicImpact, Ground, FRotator::ZeroRotator, 1.8f);
	SpawnFXAt(FX_BigKnockup, Ground, FRotator::ZeroRotator, 1.1f);
	ShakePlayerCamera(UBossShake_Heavy::StaticClass(), 1.3f);
	PlaySFX(SFX_Impact, Center, 1.2f);

	if (IsTargetInCircle(Center, LeapRadius))
	{
		FVector Away = GetTargetChar()->GetActorLocation() - Center;
		Away.Z = 0.f;
		DamagePlayer(GetConfig(ENarbashSkill::LeapSlam).DamageMultiplier, Away.GetSafeNormal(), 900.f, 460.f);
	}

	SpawnShockRing(Center, CurrentPhase >= 2 ? 750.f : 520.f, 0.6f, CurrentPhase >= 2 ? 1.f : 0.6f);

	const float LandLen = PlaySeq(Anim_LeapLand, 1.f, 0.05f, 0.15f);
	At(SkillElapsed + FMath::Max(LandLen, 0.5f), [this]() { EndSkill(true); });
}

void AMon_Narbash::StartDrumThrow()
{
	bTracking = true;
	bTrackFixedDir = false;
	TrackTurnRate = 420.f;

	const float Length = PlaySeq(Anim_Throw, GetSkillSpeed(), 0.15f, 0.15f);
	const float ReleaseTime = FMath::Max(Length * ThrowReleaseFraction, 0.3f);

	PlaySFX(SFX_EffortSwing, GetActorLocation(), 0.9f);

	At(ReleaseTime - 0.12f, [this]() { bTracking = false; });

	At(ReleaseTime, [this]()
	{
		FVector Origin = GetMesh()->GetSocketLocation(TEXT("FX_Weapon_R_Tip"));
		if (Origin.IsNearlyZero())
			Origin = GetActorLocation() + GetActorForwardVector() * 100.f + FVector(0.f, 0.f, 60.f);

		FVector Aim = GetActorForwardVector();

		if (ACharacter* Target = GetTargetChar())
		{
			const float Flight = FVector::Dist(Origin, Target->GetActorLocation()) / ThrowSpeed;
			const FVector Predicted = Target->GetActorLocation() + Target->GetVelocity() * Flight * 0.6f;
			Aim = (Predicted - Origin).GetSafeNormal();
		}

		const TArray<float> FanAngles = CurrentPhase >= 2 ? TArray<float>{ -18.f, 0.f, 18.f } : TArray<float>{ 0.f };

		for (const float Angle : FanAngles)
		{
			const FVector Dir = Aim.RotateAngleAxis(Angle, FVector::UpVector);

			FActorSpawnParameters Params;
			Params.Instigator = this;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			if (ABossProjectile* Projectile = GetWorld()->SpawnActor<ABossProjectile>(ABossProjectile::StaticClass(), Origin, Dir.Rotation(), Params))
				Projectile->Launch(Dir, ThrowSpeed, m_tInfo.iAttack * GetConfig(ENarbashSkill::DrumThrow).DamageMultiplier, 2800.f);
		}

		SpawnFXAt(FX_MaceClacks, Origin, FRotator::ZeroRotator, 0.8f);
		ShakePlayerCamera(UBossShake_Light::StaticClass(), 0.4f, 1500.f);
	});

	At(Length + 0.05f, [this]() { EndSkill(true); });
}

void AMon_Narbash::StartRampage()
{
	bTracking = false;

	const float Speed = GetSkillSpeed();
	const float IntroTime = 1.4f / Speed;
	const float SlamGap = 0.85f / Speed;

	PlaySeq(Anim_RampageIntro, 1.f, 0.2f, 0.1f);
	SpawnFXAttached(FX_EnergyBuildup, TEXT("Chest"), 1.5f);
	PlaySFX(SFX_Roar, GetActorLocation(), 1.3f);
	ShakePlayerCamera(UBossShake_Roar::StaticClass(), 0.6f);

	// 보스 주변 근접 위험 지대 예고 (첫 강타 직전에 터진다)
	TrackTelegraph(ABossTelegraph::SpawnCircle(GetWorld(), GetActorLocation(), 330.f, IntroTime, TelegraphColor));

	// 도입 동작이 끝나면 북을 두드리는 동작을 반복 재생
	At(IntroTime - 0.1f, [this]()
	{
		PlaySeq(Anim_RampageLoop, 1.4f, 0.15f, 0.1f, 8);
	});

	for (int32 SlamIndex = 0; SlamIndex < 3; ++SlamIndex)
	{
		const bool bFinal = SlamIndex == 2;

		At(IntroTime + SlamGap * SlamIndex, [this, bFinal, SlamIndex, SlamGap]()
		{
			const FVector Center = GetActorLocation();
			const FVector Ground = GetGroundLocation(Center);

			SpawnFXAt(FX_SeismicImpact, Ground, FRotator::ZeroRotator, bFinal ? 2.2f : 1.5f);
			SpawnFXAt(FX_MaceClacks, Ground, FRotator::ZeroRotator, 1.2f);

			if (bFinal)
			{
				SpawnFXAt(FX_BigKnockup, Ground, FRotator::ZeroRotator, 1.8f);
				SpawnFXAt(FX_UltEnd, Ground, FRotator::ZeroRotator, 1.4f);
			}

			ShakePlayerCamera(bFinal ? UBossShake_Roar::StaticClass() : UBossShake_Heavy::StaticClass(), bFinal ? 1.2f : 0.9f);
			PlaySFX(SFX_Impact, Center, bFinal ? 1.5f : 1.f);

			// 몸에 붙어 있으면 직접 피해
			if (IsTargetInCircle(Center, bFinal ? 380.f : 330.f))
			{
				FVector Away = GetTargetChar()->GetActorLocation() - Center;
				Away.Z = 0.f;
				DamagePlayer(GetConfig(ENarbashSkill::Rampage).DamageMultiplier * (bFinal ? 1.4f : 1.f), Away.GetSafeNormal(), bFinal ? 950.f : 650.f, bFinal ? 600.f : 320.f);
			}

			// 바깥으로 퍼지는 충격파 (지상에 있을 때만 맞는다: 점프/대시로 회피)
			SpawnShockRing(Center, bFinal ? 950.f : 750.f, 0.85f, bFinal ? 1.2f : 0.8f);

			// 다음 강타 예고
			if (!bFinal)
				TrackTelegraph(ABossTelegraph::SpawnCircle(GetWorld(), Center, SlamIndex == 1 ? 380.f : 330.f, SlamGap, TelegraphColor));
		});
	}

	const float EndTime = IntroTime + SlamGap * 2 + 0.55f;

	At(EndTime, [this]()
	{
		const float EndLen = PlaySeq(Anim_RampageEnd, 1.f, 0.1f, 0.15f);
		At(SkillElapsed + FMath::Max(EndLen, 0.6f), [this]() { EndSkill(true); });
	});
}

// =========================================================================
// 스턴
// =========================================================================
void AMon_Narbash::EnterStagger()
{
	if (bStaggered || bDead)
		return;

	if (ActiveSkill != ENarbashSkill::None)
		CancelSkill();

	bStaggered = true;
	StaggerElapsed = 0.f;
	StaggerStage = 0;
	bArmored = false;
	Poise = MaxPoise;

	GetCharacterMovement()->StopMovementImmediately();

	if (AAIController* AICon = Cast<AAIController>(GetController()))
		AICon->StopMovement();

	StaggerStartLen = PlaySeq(Anim_StunStart, 1.f, 0.08f, 0.1f);
	StaggerEndLen = Anim_StunEnd ? Anim_StunEnd->GetPlayLength() : 0.5f;

	SpawnFXAt(FX_MeleeImpact, GetActorLocation() + FVector(0.f, 0.f, 60.f), FRotator::ZeroRotator, 2.f);
	PlaySFX(SFX_Pain, GetActorLocation(), 1.3f);
	ShakePlayerCamera(UBossShake_Heavy::StaticClass(), 0.9f);

	BOSS_LOG("스턴 (강인도 파괴)");
}

void AMon_Narbash::TickStagger(float DeltaTime)
{
	StaggerElapsed += DeltaTime;

	if (StaggerStage == 0 && StaggerElapsed >= StaggerStartLen)
	{
		StaggerStage = 1;
		PlaySeq(Anim_StunLoop, 1.f, 0.1f, 0.1f, 20);
	}

	if (StaggerStage <= 1 && StaggerElapsed >= StaggerDuration - StaggerEndLen)
	{
		StaggerStage = 2;
		PlaySeq(Anim_StunEnd, 1.f, 0.1f, 0.15f);
	}

	if (StaggerElapsed >= StaggerDuration)
		EndStagger();
}

void AMon_Narbash::EndStagger()
{
	bStaggered = false;

	// 스턴에서 깨어나면 성난 듯 바로 반격한다 (짧은 후딜)
	RecoveryRemaining = 0.25f;
}

// =========================================================================
// 유틸
// =========================================================================
float AMon_Narbash::RollFeintDelay() const
{
	// 7단계 적응형 AI: 회피를 자주 하는 플레이어에게는 공격 시작을 확률적으로 늦춰 타이밍을 흐트러뜨린다
	UCombatPatternSubsystem* Subsystem = nullptr;
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
		Subsystem = GI->GetSubsystem<UCombatPatternSubsystem>();

	if (!Subsystem)
		return 0.f;

	const float DodgeRate = Subsystem->GetDodgeRate();
	const float Chance = FMath::Clamp((DodgeRate - 0.5f) / 0.5f, 0.f, 1.f) * 0.6f;

	return FMath::FRand() < Chance ? FMath::FRandRange(0.3f, 0.55f) : 0.f;
}

float AMon_Narbash::PlaySeq(UAnimSequence* Seq, float Rate, float BlendIn, float BlendOut, int32 Loops, float StartTime)
{
	CurrentDynamicMontage = nullptr;

	if (!IsValid(Seq) || !IsValid(pNarbashAnim))
		return 0.f;

	CurrentDynamicMontage = pNarbashAnim->PlaySlotAnimationAsDynamicMontage(Seq, MontageSlotName, BlendIn, BlendOut, Rate, Loops, -1.f, StartTime);

	return CurrentDynamicMontage ? Seq->GetPlayLength() / FMath::Max(Rate, 0.01f) : 0.f;
}

UParticleSystemComponent* AMon_Narbash::SpawnFXAt(UParticleSystem* FX, const FVector& Location, const FRotator& Rotation, float Scale)
{
	if (!FX)
		return nullptr;

	return UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), FX, Location, Rotation, FVector(Scale), true);
}

UParticleSystemComponent* AMon_Narbash::SpawnFXAttached(UParticleSystem* FX, FName Socket, float Scale, bool bTrackForCleanup)
{
	if (!FX)
		return nullptr;

	UParticleSystemComponent* Comp = UGameplayStatics::SpawnEmitterAttached(FX, GetMesh(), Socket, FVector::ZeroVector, FRotator::ZeroRotator, FVector(Scale), EAttachLocation::SnapToTarget, true);

	if (Comp && bTrackForCleanup)
		ActiveSkillFX.Add(Comp);

	return Comp;
}

void AMon_Narbash::PlaySFX(USoundBase* Sound, const FVector& Location, float Volume)
{
	if (!Sound)
		return;

	// 파라곤 보이스 큐는 영어 자막이 함께 출력되므로, 자막을 끈 오디오 컴포넌트로 직접 재생한다
	UAudioComponent* Comp = NewObject<UAudioComponent>(this);
	Comp->bAutoActivate = false;
	Comp->bAutoDestroy = true;
	Comp->bSuppressSubtitles = true;
	Comp->bStopWhenOwnerDestroyed = false;
	Comp->SetSound(Sound);
	Comp->SetVolumeMultiplier(Volume);
	Comp->SetupAttachment(GetRootComponent());
	Comp->RegisterComponent();
	Comp->SetWorldLocation(Location);
	Comp->Play();
}

void AMon_Narbash::ShakePlayerCamera(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale, float MaxDistance)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);

	if (!IsValid(PC) || !ShakeClass)
		return;

	// 멀리 있을수록 약하게 흔들린다
	float DistanceScale = 1.f;

	if (const APawn* Pawn = PC->GetPawn())
		DistanceScale = FMath::Clamp(1.f - FVector::Dist(Pawn->GetActorLocation(), GetActorLocation()) / MaxDistance, 0.25f, 1.f);

	PC->ClientStartCameraShake(ShakeClass, Scale * DistanceScale);
}

ABossTelegraph* AMon_Narbash::TrackTelegraph(ABossTelegraph* Telegraph)
{
	if (Telegraph)
		ActiveTelegraphs.Add(Telegraph);

	return Telegraph;
}

void AMon_Narbash::ClearSkillVisuals()
{
	for (TWeakObjectPtr<ABossTelegraph>& Telegraph : ActiveTelegraphs)
	{
		if (Telegraph.IsValid())
			Telegraph->Dismiss();
	}

	ActiveTelegraphs.Reset();

	for (TWeakObjectPtr<UParticleSystemComponent>& Comp : ActiveSkillFX)
	{
		if (Comp.IsValid())
		{
			Comp->bAutoDestroy = true;
			Comp->DeactivateSystem();
		}
	}

	ActiveSkillFX.Reset();
}

FVector AMon_Narbash::GetGroundLocation(const FVector& Location) const
{
	// 캐릭터 캡슐에 맞지 않도록 정적 지형만 대상으로 아래로 트레이스한다
	FHitResult Hit;
	FCollisionQueryParams Params(NAME_None, false, this);
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);

	if (GetWorld()->LineTraceSingleByObjectType(Hit, Location + FVector(0.f, 0.f, 150.f), Location - FVector(0.f, 0.f, 1200.f), ObjectParams, Params))
		return Hit.ImpactPoint;

	return Location - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
}

bool AMon_Narbash::IsTargetInCircle(const FVector& Center, float Radius, float MaxHeightDiff) const
{
	const ACharacter* Target = GetTargetChar();

	if (!IsValid(Target))
		return false;

	const FVector TargetLoc = Target->GetActorLocation();
	const float TargetRadius = Target->GetCapsuleComponent() ? Target->GetCapsuleComponent()->GetScaledCapsuleRadius() : 34.f;

	return FVector::Dist2D(Center, TargetLoc) <= Radius + TargetRadius && FMath::Abs(TargetLoc.Z - Center.Z) <= MaxHeightDiff;
}

float AMon_Narbash::DamagePlayer(float Multiplier, const FVector& KnockbackDir, float KnockStrength, float KnockbackUp)
{
	ACharacter* Target = GetTargetChar();

	if (!IsValid(Target))
		return 0.f;

	FDamageEvent DamageEvent;
	const float Dealt = Target->TakeDamage(m_tInfo.iAttack * Multiplier, DamageEvent, GetController(), this);

	RecordSkillResult(Dealt > 0.f);

	if (Dealt > 0.f)
	{
		bSkillConnected = true;
		Target->LaunchCharacter(KnockbackDir * KnockStrength + FVector(0.f, 0.f, KnockbackUp), true, true);
	}

	return Dealt;
}

void AMon_Narbash::PushPlayerAway(float Radius, float Strength, float Up)
{
	ACharacter* Target = GetTargetChar();

	if (!IsValid(Target))
		return;

	FVector Away = Target->GetActorLocation() - GetActorLocation();
	Away.Z = 0.f;

	if (Away.Size() > Radius)
		return;

	Target->LaunchCharacter(Away.GetSafeNormal() * Strength + FVector(0.f, 0.f, Up), true, true);
}

void AMon_Narbash::SpawnShockRing(const FVector& Center, float MaxRadius, float Duration, float DamageMultiplier)
{
	ABossTelegraph::SpawnRing(GetWorld(), Center, MaxRadius, Duration, RingColor);

	if (DamageMultiplier <= 0.f)
		return;

	FShockRing Ring;
	Ring.Center = Center;
	Ring.Duration = Duration;
	Ring.MaxRadius = MaxRadius;
	Ring.Damage = DamageMultiplier;
	ShockRings.Add(Ring);
}

void AMon_Narbash::TickShockRings(float DeltaTime)
{
	for (int32 i = ShockRings.Num() - 1; i >= 0; --i)
	{
		FShockRing& Ring = ShockRings[i];
		Ring.Elapsed += DeltaTime;

		const float Radius = Ring.MaxRadius * FMath::Clamp(Ring.Elapsed / Ring.Duration, 0.f, 1.f);

		if (!Ring.bHit)
		{
			if (ACharacter* Target = GetTargetChar())
			{
				const float Dist = FVector::Dist2D(Ring.Center, Target->GetActorLocation());

				// 링이 지나가는 순간(폭 130) 땅에 서 있으면 피해. 점프 중이거나 대시 무적이면 맞지 않는다
				if (FMath::Abs(Dist - Radius) <= 130.f && Target->GetCharacterMovement()->IsMovingOnGround())
				{
					Ring.bHit = true;

					FVector Away = Target->GetActorLocation() - Ring.Center;
					Away.Z = 0.f;
					DamagePlayer(Ring.Damage, Away.GetSafeNormal(), 450.f, 260.f);
				}
			}
		}

		if (Ring.Elapsed >= Ring.Duration)
			ShockRings.RemoveAtSwap(i);
	}
}

void AMon_Narbash::ActivateEnrageAura()
{
	if (IsValid(AuraComponent))
		return;

	// 몸 주위를 도는 파편 + 손끝의 초록 에너지: 2페이즈 내내 유지되는 광폭화 표시
	if (FX_Aura)
		AuraComponent = UGameplayStatics::SpawnEmitterAttached(FX_Aura, GetMesh(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, FVector(1.f), EAttachLocation::SnapToTarget, false);

	if (FX_AuraEnergy)
		AuraEnergyComponent = UGameplayStatics::SpawnEmitterAttached(FX_AuraEnergy, GetMesh(), TEXT("Chest"), FVector::ZeroVector, FRotator::ZeroRotator, FVector(1.f), EAttachLocation::SnapToTarget, false);
}

// =========================================================================
// 전투 서브트리 (코드로 조립)
// =========================================================================
UBehaviorTree* AMon_Narbash::GetCombatBehaviorTree(UBlackboardData* BlackboardAsset)
{
	if (!bUseCodeBuiltTree)
		return Super::GetCombatBehaviorTree(BlackboardAsset);

	if (IsValid(CodeBuiltTree))
		return CodeBuiltTree;

	// Root(Selector)
	//  ├─ [Boss Skill Ready, Lower Priority Abort] Sequence "Cast Skill" -> Boss Skill
	//  └─ Boss Approach  (사거리까지 접근 / 후딜·스턴 중 대기)
	UBehaviorTree* Tree = NewObject<UBehaviorTree>(this, TEXT("BT_NarbashBoss_Runtime"));
	Tree->BlackboardAsset = BlackboardAsset;

	UBTComposite_Selector* Root = NewObject<UBTComposite_Selector>(Tree, TEXT("NarbashCombatRoot"));
	Root->NodeName = TEXT("Narbash Combat");
	Tree->RootNode = Root;

	UBTComposite_Sequence* CastSequence = NewObject<UBTComposite_Sequence>(Tree, TEXT("CastSkillSequence"));
	CastSequence->NodeName = TEXT("Cast Skill");

	UBTDecorator_BossSkillReady* ReadyDecorator = NewObject<UBTDecorator_BossSkillReady>(Tree, TEXT("SkillReadyDecorator"));

	UBTTask_BossSkill* SkillTask = NewObject<UBTTask_BossSkill>(Tree, TEXT("BossSkillTask"));
	UBTTask_BossApproach* ApproachTask = NewObject<UBTTask_BossApproach>(Tree, TEXT("BossApproachTask"));

	FBTCompositeChild& CastChild = Root->Children.AddDefaulted_GetRef();
	CastChild.ChildComposite = CastSequence;
	CastChild.Decorators.Add(ReadyDecorator);
	CastChild.DecoratorOps.Add(FBTDecoratorLogic(EBTDecoratorLogic::Test, 0));

	FBTCompositeChild& SkillChild = CastSequence->Children.AddDefaulted_GetRef();
	SkillChild.ChildTask = SkillTask;

	FBTCompositeChild& ApproachChild = Root->Children.AddDefaulted_GetRef();
	ApproachChild.ChildTask = ApproachTask;

	CodeBuiltTree = Tree;

	BOSS_LOG("코드 조립 BT 생성 (BB=%s)", BlackboardAsset ? *BlackboardAsset->GetName() : TEXT("null"));

	return CodeBuiltTree;
}
