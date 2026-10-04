#include "Py_Grey.h"
#include "Py_GreyAnim.h"
#include "EffectDestroy.h"
#include "ItemBase.h"
#include "InventoryComponent.h"
#include "EquipmentComponent.h"
#include "UI_Equipment.h"
#include "QuestComponent.h"
#include "ExperienceComponent.h"
#include "UI_HpBar.h"
#include "UI_Inventory.h"
#include "UI_QuestTracker.h"
#include "UI_ExpBar.h"
#include "UI_SkillBar.h"
#include "UI_Minimap.h"
#include "QuestWaypoint.h"
#include "UI_QuestWaypoint.h"
#include "UI_Ending.h"
#include "UI_Death.h"
#include "StoryComponent.h"
#include "NPC_Base.h"
#include "Monster.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "AnimNotify_PlayNiagaraEffect.h"
#include "NiagaraComponent.h"
#include "DrawDebugHelpers.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraShakeBase.h"
#include "CombatPatternSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "LevelTransitionSubsystem.h"
#include "LockOnComponent.h"
#include "Misc/App.h"
#include "CapstoneSFX.h"
#include "Components/SceneCaptureComponent2D.h"

namespace
{
	// LaunchCharacter로 초속 V0을 주면 지상 마찰(k)과 고정 감속(B)에 깎여 d(V0) = V0/k - (B/k^2) * ln(1 + k*V0/B) 만큼 미끄러지고 멈춘다.
	// 캐릭터 무브먼트의 실제 마찰/감속 값으로 이 식을 이분 탐색해서, 원하는 거리만큼 정확히 파고드는 발사 속도를 구한다
	float ComputeLaunchSpeedForTravel(const UCharacterMovementComponent& Movement, float Travel)
	{
		const float Friction = (Movement.bUseSeparateBrakingFriction ? Movement.BrakingFriction : Movement.GroundFriction) * Movement.BrakingFrictionFactor;
		const float Deceleration = Movement.BrakingDecelerationWalking;

		auto TravelDistance = [Friction, Deceleration](float Speed)
		{
			if (Friction < UE_KINDA_SMALL_NUMBER)
				return Deceleration < UE_KINDA_SMALL_NUMBER ? Speed : Speed * Speed / (2.f * Deceleration);

			if (Deceleration < UE_KINDA_SMALL_NUMBER)
				return Speed / Friction;

			return Speed / Friction - (Deceleration / (Friction * Friction)) * FMath::Loge(1.f + Friction * Speed / Deceleration);
		};

		float Low = 0.f;
		float High = 8000.f;

		for (int32 Iteration = 0; Iteration < 24; ++Iteration)
		{
			const float Mid = (Low + High) * 0.5f;

			if (TravelDistance(Mid) < Travel)
				Low = Mid;
			else
				High = Mid;
		}

		return High;
	}
}

APy_Grey::FOnPlayerDodge APy_Grey::OnPlayerDodge;

APy_Grey::FOnWeaponEnhanced APy_Grey::OnWeaponEnhanced;

APy_Grey::APy_Grey()
{
	PrimaryActorTick.bCanEverTick = true;

	// 기본 검(+5 공격력)과 기본 방패(+40 체력)를 끼고 시작하면 예전과 같은 공격력 10 / 체력 200이 된다 (UEquipmentComponent::EquipDefaults)
	m_tInfo.iMaxHp = 160;
	m_tInfo.iHp = 160;
	m_tInfo.iAttack = 5;

	pArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Arm"));
	pCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));

	pArm->SetupAttachment(GetMesh());
	pCamera->SetupAttachment(pArm);

	pArm->bUsePawnControlRotation = true;
	pArm->bDoCollisionTest = true;
	pArm->ProbeChannel = ECC_Camera;
	pArm->ProbeSize = 12.f;
	
	static ConstructorHelpers::FClassFinder<UPy_GreyAnim> AP_PlayerGrey(TEXT("AnimBlueprint'/Game/UE_Asset/Character/Player/Grey/AP_PyGrey.AP_PyGrey_C'"));

	if (AP_PlayerGrey.Succeeded())
	{
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		GetMesh()->SetAnimInstanceClass(AP_PlayerGrey.Class);
	}

	WeaponBox = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponBox"));
	WeaponBox->SetupAttachment(GetMesh(), TEXT("FX_Sword_Mid"));

	// 위치는 지도를 구울 때마다 UMinimapBaker가 직접 옮기고,
	// 방향은 항상 고정(정북 방향 고정 정사영)이라 부모 회전과 무관해야 하므로 절대 위치/회전으로 둔다
	MinimapCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MinimapCaptureComponent"));
	MinimapCaptureComponent->SetupAttachment(RootComponent);
	MinimapCaptureComponent->SetAbsolute(true, true, false);
	MinimapCaptureComponent->bCaptureEveryFrame = false;
	MinimapCaptureComponent->bCaptureOnMovement = false;

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	EquipmentComponent = CreateDefaultSubobject<UEquipmentComponent>(TEXT("EquipmentComponent"));
	QuestComponent = CreateDefaultSubobject<UQuestComponent>(TEXT("QuestComponent"));
	ExperienceComponent = CreateDefaultSubobject<UExperienceComponent>(TEXT("ExperienceComponent"));
	StoryComponent = CreateDefaultSubobject<UStoryComponent>(TEXT("StoryComponent"));
	LockOnComponent = CreateDefaultSubobject<ULockOnComponent>(TEXT("LockOnComponent"));

	DeathWidgetClass = UUI_Death::StaticClass();
	EquipmentWidgetClass = UUI_Equipment::StaticClass();

	static ConstructorHelpers::FObjectFinder<UAnimSequence> DeathAnimAsset(TEXT("/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Death.Death"));

	if (DeathAnimAsset.Succeeded())
		DeathAnimation = DeathAnimAsset.Object;

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> PowerSlashSwordFXAsset(TEXT("/Game/SlashTrail_SoftTofu/Niagara/Fire/NS_Sword_Fire.NS_Sword_Fire"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> PowerSlashTrailFXAsset(TEXT("/Game/SlashTrail_SoftTofu/Niagara/Fire/NS_SlashTrail_Fire_Loop.NS_SlashTrail_Fire_Loop"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> PowerSlashHitFXAsset(TEXT("/Game/SlashTrail_SoftTofu/Niagara/Fire/NS_Hit_Fire.NS_Hit_Fire"));

	if (PowerSlashSwordFXAsset.Succeeded())
		PowerSlashSwordFX = PowerSlashSwordFXAsset.Object;

	if (PowerSlashTrailFXAsset.Succeeded())
		PowerSlashTrailFX = PowerSlashTrailFXAsset.Object;

	if (PowerSlashHitFXAsset.Succeeded())
		PowerSlashHitFX = PowerSlashHitFXAsset.Object;

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> DashFXAsset(TEXT("/Game/BlinkAndDashVFX/VFX_Niagara/NS_Dash_Mana.NS_Dash_Mana"));

	if (DashFXAsset.Succeeded())
		DashEffect = DashFXAsset.Object;
}


void APy_Grey::BeginPlay()
{
	Super::BeginPlay();

	pGreyAnim = Cast<UPy_GreyAnim>(GetMesh()->GetAnimInstance());

	DefaultBrakingDecelerationFalling = GetCharacterMovement()->BrakingDecelerationFalling;
	DefaultFallingLateralFriction = GetCharacterMovement()->FallingLateralFriction;


	SuppressMontageDashEffects();

	// 카메라 펀치가 되돌아갈 기준값. 블루프린트에서 조정한 FOV/위치를 그대로 존중한다
	if (pCamera)
	{
		BaseCameraFOV = pCamera->FieldOfView;
		BaseCameraRelativeLocation = pCamera->GetRelativeLocation();
	}

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Player"));
	WeaponBox->SetCollisionProfileName(TEXT("PlayerAttack"));

	WeaponBox->OnComponentBeginOverlap.AddDynamic(this, &APy_Grey::WepaonBeginOverlap);
	WeaponBox->OnComponentEndOverlap.AddDynamic(this, &APy_Grey::WeaponEndOverlap);

	// 포탈(OpenLevel)로 이동하면 새 폰이 PlayerStart 위치에 곧바로 스폰되는데, 언리얼은 스폰 시점에
	// 이미 겹쳐 있는 콜리전에 대해서는 BeginOverlap 이벤트를 쏘지 않는다. PlayerStart가 NPC의
	// InteractionSphere나 채집물의 CollisionComponent 안쪽에 있으면(예: NPC 바로 옆으로 복귀하는 태그)
	// OnOverlapBegin이 영영 호출되지 않아 CurrNPC/CurrItem이 비어 있고 상호작용(E)이 먹통이 된다.
	// BeginPlay에서 한 번 직접 겹침을 검사해 이 경우를 보정한다.
	TArray<AActor*> OverlappingActors;
	GetCapsuleComponent()->GetOverlappingActors(OverlappingActors);
	for (AActor* Overlapping : OverlappingActors)
	{
		if (ANPC_Base* NPC = Cast<ANPC_Base>(Overlapping))
		{
			CurrNPC = NPC;
		}
		else if (AItemBase* Item = Cast<AItemBase>(Overlapping))
		{
			CurrItem = Item;
		}
	}

	WeaponBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponBox->SetBoxExtent({ 7.5f, 2.5f, 60.f});

	bool bRestoredEquipment = false;

	// 포탈을 통해 방금 다른 레벨에서 넘어온 경우, 파괴되기 전 폰이 맡겨둔 체력을 복원한다
	// (인벤토리/경험치와 동일한 패턴 — 안 하면 OpenLevel로 폰이 새로 생성될 때마다 생성자의 풀피로 되돌아감)
	if (UGameInstance* GI = GetGameInstance())
	{
		if (ULevelTransitionSubsystem* TransitionSubsystem = GI->GetSubsystem<ULevelTransitionSubsystem>())
		{
			int32 SavedMaxHp = 0;
			int32 SavedCurrentHp = 0;
			if (TransitionSubsystem->ConsumePendingHp(SavedMaxHp, SavedCurrentHp))
			{
				m_tInfo.iMaxHp = SavedMaxHp;
				m_tInfo.iHp = SavedCurrentHp;
			}

			// 레벨업/무기 강화로 오른 공격력과 강화 단계도 같이 복원한다 (강화 비용/최대 단계 판정이 WeaponLevel에 의존)
			int32 SavedAttack = 0;
			int32 SavedWeaponLevel = 0;
			if (TransitionSubsystem->ConsumePendingCombatStats(SavedAttack, SavedWeaponLevel))
			{
				m_tInfo.iAttack = SavedAttack;
				WeaponLevel = SavedWeaponLevel;
			}

			// 장착 중이던 검/방패는 모습만 복원한다 (그 보너스는 위에서 복원한 공격력/체력에 이미 들어 있다)
			TArray<FName> SavedEquipment;
			if (EquipmentComponent && TransitionSubsystem->ConsumePendingEquipment(SavedEquipment))
			{
				EquipmentComponent->RestoreEquipped(SavedEquipment);
				bRestoredEquipment = true;
			}
		}
	}

	// 레벨을 거치지 않고 새로 시작한 경우: 기본 검과 방패를 끼고 그 보너스를 더한다
	if (EquipmentComponent && !bRestoredEquipment)
	{
		EquipmentComponent->EquipDefaults();
	}

	if (EquipmentComponent)
	{
		EquipmentComponent->OnEquipmentChanged.AddDynamic(this, &APy_Grey::HandleEquipmentChanged);
	}

	if (IsValid(PlayerHpBarClass))
	{
		PlayerHpBar = CreateWidget<UUI_HpBar>(GetWorld(), PlayerHpBarClass);

		if (IsValid(PlayerHpBar))
		{
			PlayerHpBar->AddToViewport();

			PlayerHpBar->SetHpValue(m_tInfo.iHp, m_tInfo.iMaxHp);
		}
	}

	if (IsValid(QuestTrackerWidgetClass))
	{
		QuestTrackerWidget = CreateWidget<UUI_QuestTracker>(GetWorld(), QuestTrackerWidgetClass);

		if (IsValid(QuestTrackerWidget))
		{
			QuestTrackerWidget->AddToViewport();
			QuestTrackerWidget->ClearQuest();
		}
	}

	if (QuestComponent)
	{
		QuestComponent->OnQuestTrackerChanged.AddDynamic(this, &APy_Grey::RefreshQuestTracker);
		// 이전 레벨에서 복원된 퀘스트가 있을 수 있으므로 바인딩 직후 한 번 동기화
		RefreshQuestTracker();
	}

	if (IsValid(ExpBarWidgetClass))
	{
		ExpBarWidget = CreateWidget<UUI_ExpBar>(GetWorld(), ExpBarWidgetClass);

		if (IsValid(ExpBarWidget))
		{
			ExpBarWidget->AddToViewport();
		}
	}

	if (ExperienceComponent)
	{
		ExperienceComponent->OnExpChanged.AddDynamic(this, &APy_Grey::RefreshExpBar);
		ExperienceComponent->OnLevelUp.AddDynamic(this, &APy_Grey::HandleLevelUp);

		RefreshExpBar();
	}

	if (IsValid(SkillBarWidgetClass))
	{
		SkillBarWidget = CreateWidget<UUI_SkillBar>(GetWorld(), SkillBarWidgetClass);

		if (IsValid(SkillBarWidget))
		{
			SkillBarWidget->AddToViewport();
			SkillBarWidget->InitSkillBar(this);
		}
	}

	if (IsValid(MinimapWidgetClass))
	{
		MinimapWidget = CreateWidget<UUI_Minimap>(GetWorld(), MinimapWidgetClass);

		if (IsValid(MinimapWidget))
		{
			MinimapWidget->AddToViewport();
			MinimapWidget->InitMinimap(this);

			LOG(TEXT("미니맵 위젯 생성 및 AddToViewport 완료 (클래스=%s)."), *MinimapWidgetClass->GetName());
		}
		else
		{
			LOG(TEXT("MinimapWidgetClass는 지정됐지만 CreateWidget이 실패했습니다 (WBP_Minimap의 Parent Class가 UI_Minimap인지 확인)."));
		}
	}
	else
	{
		LOG(TEXT("MinimapWidgetClass가 비어 있어 미니맵이 생성되지 않았습니다 (이 폰의 블루프린트 Class Defaults에서 Minimap Widget Class를 WBP_Minimap으로 지정하세요)."));
	}

	if (IsValid(QuestWaypointWidgetClass))
	{
		// 위젯 클래스를 먼저 지정한 뒤 FinishSpawning으로 컴포넌트를 등록해야 위젯이 올바른 클래스로 만들어진다
		const FTransform SpawnTransform(GetActorLocation());
		QuestWaypoint = GetWorld()->SpawnActorDeferred<AQuestWaypoint>(AQuestWaypoint::StaticClass(), SpawnTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		if (IsValid(QuestWaypoint))
		{
			QuestWaypoint->InitWaypoint(this, QuestWaypointWidgetClass);
			QuestWaypoint->FinishSpawning(SpawnTransform);
		}
	}
	else
	{
		LOG(TEXT("QuestWaypointWidgetClass가 비어 있어 퀘스트 위치 마커가 생성되지 않았습니다 (이 폰의 블루프린트 Class Defaults에서 Quest Waypoint Widget Class를 WBP_QuestWaypoint로 지정하세요)."));
	}

	BossDefeatedHandle = AMonster::OnBossDefeated.AddUObject(this, &APy_Grey::HandleBossDefeated);

}

void APy_Grey::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AMonster::OnBossDefeated.Remove(BossDefeatedHandle);

	RestoreMontageDashEffects();

	// 히트스톱 도중 폰이 사라져도 전역 시간이 느려진 채로 남지 않게 한다
	HitStopRealEnd = 0.0;
	UpdateTimeDilation();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackBuffTimerHandle);
		World->GetTimerManager().ClearTimer(ShieldTimerHandle);
		World->GetTimerManager().ClearTimer(RespawnTimerHandle);

		for (FTimerHandle& ReadyTimer : SkillReadyTimers)
			World->GetTimerManager().ClearTimer(ReadyTimer);

		EndPowerSlash();
	}

	if (IsValid(QuestWaypoint))
	{
		QuestWaypoint->Destroy();
		QuestWaypoint = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}


void APy_Grey::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bPowerSlashSwinging)
		UpdatePowerSlashHit();

	UpdateTimeDilation();

	// 히트스톱으로 게임 시간이 거의 멈춰도 카메라 펀치는 굳지 않고 실시간으로 흘러야 한다
	UpdateCameraPunch(FApp::GetDeltaTime());

	if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())
		LastFallSpeedZ = GetVelocity().Z;

	UpdateLowHealthHeartbeat();
}

void APy_Grey::UpdateLowHealthHeartbeat()
{
	const float Ratio = GetHpRatio();

	if (bIsDead || Ratio <= 0.f || Ratio > 0.3f)
	{
		HeartbeatTimer = 0.f;
		return;
	}

	// 히트스톱/슬로모션에 영향받지 않는 실시간 타이머
	HeartbeatTimer -= FApp::GetDeltaTime();

	if (HeartbeatTimer > 0.f)
		return;

	// 체력이 낮을수록 빠르고 크게 뛴다 (30% -> 1.3초 간격, 5% -> 0.75초 간격)
	HeartbeatTimer = FMath::GetMappedRangeValueClamped(FVector2D(0.3f, 0.05f), FVector2D(1.3f, 0.75f), Ratio);
	CapstoneSFX::Play2D(this, ESFX::Player_Heartbeat, FMath::GetMappedRangeValueClamped(FVector2D(0.3f, 0.05f), FVector2D(0.5f, 1.f), Ratio));
}

void APy_Grey::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	CapstoneSFX::Play2D(this, ESFX::Player_Jump);

	if (FMath::FRand() < 0.4f)
		CapstoneSFX::Play2D(this, ESFX::Player_VoiceJump);
}

void APy_Grey::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// 계단/작은 턱 정도의 낙하는 소리를 내지 않는다
	if (LastFallSpeedZ < -450.f)
	{
		const float Volume = FMath::Clamp(-LastFallSpeedZ / 1200.f, 0.5f, 1.3f);
		const float HalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;
		CapstoneSFX::PlayAt(this, ESFX::Player_Land, GetActorLocation() - FVector(0.f, 0.f, HalfHeight), Volume);
	}

	LastFallSpeedZ = 0.f;
}

void  APy_Grey::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveFront"), this, &APy_Grey::MoveFront);
	PlayerInputComponent->BindAxis(TEXT("MoveSide"), this, &APy_Grey::MoveSide);
	PlayerInputComponent->BindAxis(TEXT("RotationZ"), this, &APy_Grey::RotationZ);
	PlayerInputComponent->BindAxis(TEXT("RotationX"), this, &APy_Grey::RotationX);
	

	PlayerInputComponent->BindAction(TEXT("JumpKey"),EInputEvent::IE_Pressed,this, &APy_Grey::JumpKey);
	PlayerInputComponent->BindAction(TEXT("NormalAttack"), EInputEvent::IE_Pressed, this, &APy_Grey::NormalAttack);

	PlayerInputComponent->BindAction(TEXT("SkillSlotA"), EInputEvent::IE_Pressed, this, &APy_Grey::SkillSlotA);
	PlayerInputComponent->BindAction(TEXT("SkillSlotB"), EInputEvent::IE_Pressed, this, &APy_Grey::SkillSlotB);
	PlayerInputComponent->BindAction(TEXT("SkillSlotC"), EInputEvent::IE_Pressed, this, &APy_Grey::SkillSlotC);

	PlayerInputComponent->BindAction(TEXT("Interact"), EInputEvent::IE_Pressed, this, &APy_Grey::OnInteractKeyPressed);
	PlayerInputComponent->BindAction(TEXT("Inventory"), EInputEvent::IE_Pressed, this, &APy_Grey::ToggleInventory);
	PlayerInputComponent->BindAction(TEXT("Equipment"), EInputEvent::IE_Pressed, this, &APy_Grey::ToggleEquipment);
	PlayerInputComponent->BindAction(TEXT("UsePotion"), EInputEvent::IE_Pressed, this, &APy_Grey::UsePotion);

	PlayerInputComponent->BindAction(TEXT("RollOrDash"), EInputEvent::IE_Pressed, this, &APy_Grey::RollOrDash);

	PlayerInputComponent->BindAction(TEXT("LockOn"), EInputEvent::IE_Pressed, this, &APy_Grey::LockOnKey);
	PlayerInputComponent->BindAction(TEXT("LockOnSwitchNext"), EInputEvent::IE_Pressed, this, &APy_Grey::LockOnSwitchNext);
	PlayerInputComponent->BindAction(TEXT("LockOnSwitchPrev"), EInputEvent::IE_Pressed, this, &APy_Grey::LockOnSwitchPrev);
}

void APy_Grey::SetGameplayInputLocked(bool bLocked)
{
	bGameplayInputLocked = bLocked;

	// UI/대사/사망 중에 카메라가 대상을 붙잡고 있으면 안 되므로 락온을 푼다
	if (bLocked && LockOnComponent)
		LockOnComponent->ReleaseLock();
}

void APy_Grey::SetNPCPanelOpen(bool bOpen)
{
	bNPCPanelOpen = bOpen;
	RefreshPanelInputMode();
}

void APy_Grey::RefreshPanelInputMode()
{
	const bool bAnyPanelOpen = bNPCPanelOpen
		|| (IsValid(InventoryWidget) && InventoryWidget->IsInViewport())
		|| (IsValid(EquipmentWidget) && EquipmentWidget->IsInViewport());

	SetGameplayInputLocked(bAnyPanelOpen);

	APlayerController* PC = Cast<APlayerController>(GetController());

	if (!PC)
		return;

	if (bAnyPanelOpen)
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeGameAndUI());
	}
	else
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
		// UI가 떠 있는 동안 마우스 클릭(NormalAttack 키) 등이 눌린 채로 걸려 있을 수 있어 초기화
		PC->FlushPressedKeys();
	}
}

void APy_Grey::LockOnKey()
{
	if (bIsDead || bGameplayInputLocked || !LockOnComponent)
		return;

	LockOnComponent->ToggleLockOn();
}

void APy_Grey::LockOnSwitchNext()
{
	if (bIsDead || bGameplayInputLocked || !LockOnComponent)
		return;

	LockOnComponent->SwitchTarget(1);
}

void APy_Grey::LockOnSwitchPrev()
{
	if (bIsDead || bGameplayInputLocked || !LockOnComponent)
		return;

	LockOnComponent->SwitchTarget(-1);
}

void APy_Grey::SetExplosiveBuffState(bool bIsActive)
{
	bIsExplosiveBuffActive = bIsActive;
}

void APy_Grey::ActivateShield(int32 Amount, float Duration, AEffectDestroy* ShieldEffect)
{
	if (Amount <= 0)
		return;

	// 재사용하면 수치/지속시간이 새로 시작되고, 이전 보호막의 VFX는 겹치지 않게 걷어낸다
	if (AEffectDestroy* OldEffect = ShieldEffectActor.Get())
	{
		if (OldEffect != ShieldEffect)
			OldEffect->Destroy();
	}

	ShieldEffectActor = ShieldEffect;
	CurrentShield = Amount;
	MaxShield = Amount;

	// 노티파이에서 bAttachToTarget을 꺼둔 경우에도 보호막이 이동하는 플레이어를 따라다니도록 보정
	if (IsValid(ShieldEffect) && ShieldEffect->GetRootComponent() && !ShieldEffect->GetAttachParentActor())
	{
		ShieldEffect->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
	}

	if (Duration > 0.f)
	{
		GetWorldTimerManager().SetTimer(ShieldTimerHandle, this, &APy_Grey::EndShield, Duration, false);
	}
	else
	{
		GetWorldTimerManager().ClearTimer(ShieldTimerHandle);
	}

	RefreshShieldUI();

	CapstoneSFX::Play2D(this, ESFX::Player_ShieldUp);

	LOG(TEXT("스킬 B 보호막 시작 : %d, %.1f초"), Amount, Duration);
}

void APy_Grey::EndShield()
{
	GetWorldTimerManager().ClearTimer(ShieldTimerHandle);

	CurrentShield = 0.f;
	MaxShield = 0.f;

	if (AEffectDestroy* Effect = ShieldEffectActor.Get())
	{
		Effect->Destroy();
	}
	ShieldEffectActor.Reset();

	RefreshShieldUI();

	LOG(TEXT("스킬 B 보호막 종료"));
}

void APy_Grey::RefreshShieldUI()
{
	if (IsValid(PlayerHpBar))
	{
		PlayerHpBar->SetShieldValue(CurrentShield, MaxShield);
	}
}

void APy_Grey::MoveFront(float _fScale)
{
	AddMovementInput(GetActorForwardVector(), _fScale);

	if (IsValid(pGreyAnim))
	{
		if (_fScale > 0.f)
			pGreyAnim->SetMoveDir(EDir::Front);

		else if (_fScale < 0.f)
			pGreyAnim->SetMoveDir(EDir::Back);
		
	}

}

void APy_Grey::MoveSide(float _fScale)
{
	AddMovementInput(GetActorRightVector(), _fScale);

	if (IsValid(pGreyAnim))
	{
		if (_fScale > 0.f)
			pGreyAnim->SetMoveDir(EDir::Right);

		else if (_fScale < 0.f)
			pGreyAnim->SetMoveDir(EDir::Left);

	}
}

void APy_Grey::RotationZ(float _fScale)
{
	// 락온 중에는 카메라가 대상에 고정되므로 좌우 마우스 입력은 대상 전환(플릭) 판정에만 쓴다
	if (LockOnComponent && LockOnComponent->ConsumeLookInput(_fScale))
		return;

	AddControllerYawInput(_fScale);
}

void APy_Grey::RotationX(float _fScale)
{
	if (LockOnComponent && LockOnComponent->IsLockedOn())
		return;

	AddControllerPitchInput(_fScale);
}

void APy_Grey::JumpKey()
{
	if (bIsDead)
		return;

	if (IsValid(pGreyAnim))
		pGreyAnim->JumpKey();

	Jump();
}

void APy_Grey::NormalAttack()
{
	if (bGameplayInputLocked)
		return;

	if (IsValid(pGreyAnim))
		pGreyAnim->NormalAttack();
}

void APy_Grey::SkillSlotA()
{
	if (bGameplayInputLocked)
		return;

	if (!TryUseSkill(0))
		return;

	CapstoneSFX::Play2D(this, ESFX::Player_SkillA);
	CapstoneSFX::Play2D(this, ESFX::Player_VoiceSkill);

	ActivateAttackBuff();

	if (IsValid(pGreyAnim))
		pGreyAnim->SkillSlotA();
}

void APy_Grey::ActivateAttackBuff()
{
	bAttackBuffActive = true;

	GetWorldTimerManager().SetTimer(AttackBuffTimerHandle, this, &APy_Grey::EndAttackBuff, AttackBuffDuration, false);

	LOG(TEXT("스킬 A 공격력 강화 시작 : x%.2f, %.1f초 (공격력 %d -> %.0f)"), AttackBuffMultiplier, AttackBuffDuration, m_tInfo.iAttack, GetEffectiveAttackPower());
}

void APy_Grey::EndAttackBuff()
{
	bAttackBuffActive = false;

	LOG(TEXT("스킬 A 공격력 강화 종료"));
}

float APy_Grey::GetEffectiveAttackPower() const
{
	// m_tInfo.iAttack 자체는 건드리지 않고 타격 시점에만 배율을 곱한다.
	// 버프 도중 레벨업/강화로 공격력이 바뀌어도 버프가 끝날 때 값이 꼬이지 않는다
	const float Multiplier = bAttackBuffActive ? AttackBuffMultiplier : 1.f;

	return FMath::RoundToFloat(m_tInfo.iAttack * Multiplier);
}

float APy_Grey::RollAttackDamage() const
{
	const int32 Variance = FMath::RoundToInt(DamageVariance);

	return FMath::Max(1.f, GetEffectiveAttackPower() + FMath::RandRange(-Variance, Variance));
}

void APy_Grey::SkillSlotB()
{
	if (bGameplayInputLocked)
		return;

	if (!TryUseSkill(1))
		return;

	CapstoneSFX::Play2D(this, ESFX::Player_VoiceSkill);

	if (IsValid(pGreyAnim))
	{
		pGreyAnim->SkillSlotB();
	}
}

void APy_Grey::SkillSlotC()
{
	if (bGameplayInputLocked)
		return;

	if (!TryUseSkill(2))
		return;

	CapstoneSFX::Play2D(this, ESFX::Player_SkillC);
	CapstoneSFX::Play2D(this, ESFX::Player_VoiceSkillHeavy);

	BeginPowerSlash();

	if (IsValid(pGreyAnim))
		pGreyAnim->SkillSlotC();
}

void APy_Grey::OnInteractKeyPressed()
{
	if (StoryComponent && StoryComponent->IsPlaying())
	{
		StoryComponent->AdvanceLine();
		return; // 대사 중에는 아이템 줍기/NPC 상호작용을 하지 않는다
	}

	if (CurrItem)
	{
		AItemBase* Item = CurrItem;
		Item->PickupItem(this);

		// 가방이 가득 차 못 주웠으면 아이템이 그대로 남아 있으므로 자리를 비운 뒤 다시 누를 수 있게 둔다
		if (!IsValid(Item) || Item->IsActorBeingDestroyed())
			CurrItem = nullptr;
	}

	if (CurrNPC)
	{
		CurrNPC->Interact(this);
	}
}

void APy_Grey::ToggleInventory()
{
	if (!InventoryWidgetClass)
		return;

	if (!InventoryWidget)
	{
		InventoryWidget = CreateWidget<UUI_Inventory>(GetWorld(), InventoryWidgetClass);
	}

	if (!InventoryWidget)
		return;

	if (InventoryWidget->IsInViewport())
	{
		InventoryWidget->RemoveFromParent();
		CapstoneSFX::Play2D(this, ESFX::UI_Close);
	}
	else
	{
		InventoryWidget->AddToViewport();
		InventoryWidget->PlaceInViewport();
		CapstoneSFX::Play2D(this, ESFX::UI_Open);

		if (InventoryComponent)
		{
			InventoryWidget->RefreshInventoryUI(InventoryComponent);
		}
	}

	RefreshPanelInputMode();
}

void APy_Grey::ToggleEquipment()
{
	if (bIsDead || !EquipmentComponent || !IsValid(EquipmentWidgetClass))
		return;

	APlayerController* PC = Cast<APlayerController>(GetController());

	if (!PC)
		return;

	if (!IsValid(EquipmentWidget))
	{
		EquipmentWidget = CreateWidget<UUI_Equipment>(PC, EquipmentWidgetClass);

		if (!IsValid(EquipmentWidget))
			return;

		EquipmentWidget->InitEquipment(this);
		EquipmentWidget->OnCloseRequested.AddUObject(this, &APy_Grey::ToggleEquipment);
	}

	if (EquipmentWidget->IsInViewport())
	{
		EquipmentWidget->RemoveFromParent();
		CapstoneSFX::Play2D(this, ESFX::UI_Close);
	}
	else
	{
		EquipmentWidget->AddToViewport();
		EquipmentWidget->PlaceInViewport();
		CapstoneSFX::Play2D(this, ESFX::UI_Open);
	}

	RefreshPanelInputMode();
}

void APy_Grey::HandleEquipmentChanged()
{
	if (IsValid(PlayerHpBar))
	{
		PlayerHpBar->SetHpValue(m_tInfo.iHp, m_tInfo.iMaxHp);
	}
}

void APy_Grey::UsePotion()
{
	if (!InventoryComponent)
		return;

	for (const FInventorySlot& Slot : InventoryComponent->Items)
	{
		const FItemData* Data = InventoryComponent->GetItemData(Slot.ItemID);

		if (!Data || Data->ItemType != EItemType::Potion)
			continue;

		UsePotionItem(Slot.ItemID);
		return;
	}
}

void APy_Grey::UsePotionItem(FName ItemID)
{
	if (!InventoryComponent)
		return;

	const FItemData* Data = InventoryComponent->GetItemData(ItemID);
	if (!Data)
	{
		LOG(TEXT("UsePotionItem(%s) 실패 : ItemDataTable에서 해당 ItemID를 찾지 못함"), *ItemID.ToString());
		return;
	}

	if (Data->ItemType != EItemType::Potion)
	{
		LOG(TEXT("UsePotionItem(%s) 실패 : ItemType이 Potion이 아님 (현재 값 %d)"), *ItemID.ToString(), (int32)Data->ItemType);
		CapstoneSFX::Play2D(this, ESFX::UI_Deny); // 사용할 수 없는 아이템
		return;
	}

	if (!InventoryComponent->RemoveItem(ItemID, 1))
	{
		LOG(TEXT("UsePotionItem(%s) 실패 : 인벤토리에서 제거 실패(수량 부족)"), *ItemID.ToString());
		return;
	}

	int32 HpBeforeHeal = m_tInfo.iHp;

	// 7단계 적응형 AI: 회복 시점의 HP 비율을 기록해 "저HP에 회복하는 습관"을 학습
	if (UCombatPatternSubsystem* Subsystem = UGameplayStatics::GetGameInstance(this)->GetSubsystem<UCombatPatternSubsystem>())
	{
		Subsystem->RecordPlayerPotionUse(GetHpRatio());
	}

	Heal(Data->HealAmount);
	CapstoneSFX::Play2D(this, ESFX::UI_Potion);

	LOG(TEXT("UsePotionItem(%s) 사용 : HealAmount=%d, HP %d -> %d / %d"), *ItemID.ToString(), Data->HealAmount, HpBeforeHeal, m_tInfo.iHp, m_tInfo.iMaxHp);

	if (IsValid(PlayerHpBar))
	{
		PlayerHpBar->SetHpValue(m_tInfo.iHp, m_tInfo.iMaxHp);
	}
}

void APy_Grey::RollOrDash()
{
	if (bIsDead)
		return;

	FVector InputDirection = GetCharacterMovement()->GetLastInputVector();
	FVector ActorForward = GetActorForwardVector();
	FVector ActorRight = GetActorRightVector();

	UAnimMontage* MontageToPlay = DashForwardMontage;

	if (InputDirection.IsNearlyZero())
	{
		InputDirection = ActorForward;
	}
	else
	{
		InputDirection.Normalize();

		float ForwardDot = FVector::DotProduct(ActorForward, InputDirection);
		float RightDot = FVector::DotProduct(ActorRight, InputDirection);

		if (FMath::Abs(ForwardDot) >= FMath::Abs(RightDot))
		{
			// 앞뒤 판별 후 변수에 할당
			MontageToPlay = (ForwardDot > 0.f) ? DashForwardMontage : DashBackwardMontage;
		}
		else
		{
			// 좌우 판별 후 변수에 할당
			MontageToPlay = (RightDot > 0.f) ? DashRightMontage : DashLeftMontage;
		}
	}

	// 2. 선택된 애니메이션 재생
	if (MontageToPlay)
	{
		PlayAnimMontage(MontageToPlay);
	}

	// 4. 캐릭터 발사
	// 공중(단차 낙하/점프 중)에서는 기존 수직 속도를 덮어쓰지 않아야 체공 시간이 늘어나지 않는다
	const bool bIsFalling = GetCharacterMovement()->IsFalling();
	const float LaunchSpeed = bIsFalling ? 8000.f * AirDashSpeedMultiplier : 8000.f;
	LaunchCharacter(InputDirection * LaunchSpeed, true, !bIsFalling);

	StartDashEffect(InputDirection);

	// 공중에서는 기본적으로 FallingLateralFriction=0, BrakingDecelerationFalling=0이라 대시 속도가
	// 착지할 때까지 전혀 줄지 않는다. 지상 대시가 자연스럽게 느껴지는 이유는 BrakingDecelerationWalking뿐
	// 아니라 속도에 비례해 급격히 감속시키는 GroundFriction 항 덕분이므로, 숫자를 새로 튜닝하는 대신
	// 대시 동안만 지상과 동일한 감속/마찰 값을 그대로 빌려쓴다.
	// bIsFalling이 아니라 항상 걸어야 하는 이유: 대시 시작 시점엔 땅 위여서 false지만, 단차 끝으로
	// 대시하면 도중에 공중으로 전환된다 - 그때는 이미 이 함수를 다시 타지 않으므로 기본값(0)이 남아
	// 브레이크 없이 계속 날아간다. 그래서 시작 상태와 무관하게 미리 걸어두고 대시가 끝나면 되돌린다.
	GetCharacterMovement()->BrakingDecelerationFalling = GetCharacterMovement()->BrakingDecelerationWalking;
	GetCharacterMovement()->FallingLateralFriction = GetCharacterMovement()->GroundFriction;
	GetWorldTimerManager().SetTimer(AirDashBrakingTimerHandle, this, &APy_Grey::RestoreAirBraking, InvincibleDuration, false);

	bInvincible = true;
	GetWorldTimerManager().SetTimer(InvincibilityTimerHandle, this, &APy_Grey::EndInvincibility, InvincibleDuration, false);

	OnPlayerDodge.Broadcast();
	LOG(TEXT("OnPlayerDodge 방송"));
}

void APy_Grey::EndInvincibility()
{
	bInvincible = false;
}

void APy_Grey::RestoreAirBraking()
{
	GetCharacterMovement()->BrakingDecelerationFalling = DefaultBrakingDecelerationFalling;
	GetCharacterMovement()->FallingLateralFriction = DefaultFallingLateralFriction;
}

void APy_Grey::SuppressMontageDashEffects()
{
	if (!DashEffect)
		return;

	// 대시 이펙트는 Paragon Jog_*_Dash 시퀀스에 AnimNotify_PlayNiagaraEffect로 박혀 있다. 이 노티파이는 Template이 비어 있으면
	// 아무것도 재생하지 않으므로, 에셋을 저장하지 않고 메모리상의 Template만 비워서 끈다 (EndPlay에서 복원)
	const TArray<UAnimMontage*> DashMontages = { DashForwardMontage, DashBackwardMontage, DashLeftMontage, DashRightMontage };

	for (UAnimMontage* Montage : DashMontages)
	{
		if (!Montage)
			continue;

		TArray<UAnimSequenceBase*> Sources;
		Sources.Add(Montage);

		for (const FSlotAnimationTrack& Track : Montage->SlotAnimTracks)
		{
			for (const FAnimSegment& Segment : Track.AnimTrack.AnimSegments)
			{
				if (UAnimSequenceBase* Sequence = Segment.GetAnimReference())
					Sources.AddUnique(Sequence);
			}
		}

		for (UAnimSequenceBase* Source : Sources)
		{
			for (FAnimNotifyEvent& NotifyEvent : Source->Notifies)
			{
				UAnimNotify_PlayNiagaraEffect* NiagaraNotify = Cast<UAnimNotify_PlayNiagaraEffect>(NotifyEvent.Notify);
				if (!NiagaraNotify || !NiagaraNotify->Template)
					continue;

				SuppressedDashNotifies.Emplace(NiagaraNotify, NiagaraNotify->Template);
				NiagaraNotify->Template = nullptr;
			}
		}
	}
}

void APy_Grey::RestoreMontageDashEffects()
{
	for (const TPair<TWeakObjectPtr<UAnimNotify_PlayNiagaraEffect>, TWeakObjectPtr<UNiagaraSystem>>& Suppressed : SuppressedDashNotifies)
	{
		if (Suppressed.Key.IsValid() && Suppressed.Value.IsValid())
			Suppressed.Key->Template = Suppressed.Value.Get();
	}

	SuppressedDashNotifies.Reset();
}

void APy_Grey::StartDashEffect(const FVector& Direction)
{
	if (!DashEffect)
		return;

	// 실제 발사 방향을 그대로 쓴다. 몽타주는 앞/뒤/좌/우 4방향 중 가장 가까운 것만 고르므로 대각선 대시에서는 몽타주 방향과 어긋난다
	FVector DashDirection = Direction.GetSafeNormal2D();
	if (DashDirection.IsNearlyZero())
		DashDirection = GetActorForwardVector().GetSafeNormal2D();

	const FRotator EffectRotation = DashDirection.Rotation() + FRotator(0.f, DashEffectYawOffset, 0.f);
	const FRotator OffsetRotation = DashDirection.Rotation();

	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector FeetLocation = GetActorLocation() - FVector(0.f, 0.f, HalfHeight);
	FVector SpawnLocation = FeetLocation + OffsetRotation.RotateVector(DashEffectOffset);

	// 출발점 뒤쪽에 벽이 바짝 붙어 있으면 이펙트가 벽 속에 박히므로, 캐릭터 몸통에서 스폰 지점까지 선을 쏴서 벽 앞까지만 당긴다
	FHitResult WallHit;
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(DashEffectWallTrace), false, this);
	const FVector TraceStart = GetActorLocation();
	const FVector TraceEnd = FVector(SpawnLocation.X, SpawnLocation.Y, TraceStart.Z);
	if (GetWorld()->LineTraceSingleByChannel(WallHit, TraceStart, TraceEnd, ECC_Visibility, TraceParams))
	{
		constexpr float WallMargin = 20.f;
		SpawnLocation.X = WallHit.ImpactPoint.X + WallHit.ImpactNormal.X * WallMargin;
		SpawnLocation.Y = WallHit.ImpactPoint.Y + WallHit.ImpactNormal.Y * WallMargin;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DashEffect, SpawnLocation, EffectRotation, DashEffectScale);
}

void APy_Grey::RefreshQuestTracker()
{
	if (!IsValid(QuestTrackerWidget) || !QuestComponent)
		return;

	if (QuestComponent->HasActiveQuest())
	{
		QuestTrackerWidget->UpdateQuest(QuestComponent->GetActiveQuest());
	}
	else
	{
		QuestTrackerWidget->ClearQuest();
	}
}

void APy_Grey::RefreshExpBar()
{
	if (!IsValid(ExpBarWidget) || !ExperienceComponent)
		return;

	ExpBarWidget->SetExp(ExperienceComponent->GetCurrentExp(), ExperienceComponent->GetRequiredExp(), ExperienceComponent->GetCurrentLevel());
}

void APy_Grey::HandleLevelUp(int32 NewLevel)
{
	// 체력 증가폭을 최대치/현재치에 동일하게 더한다 -> 다치지 않은 상태였으면 그대로 풀피로,
	// 깎여 있었으면 늘어난 만큼만 현재 체력에 반영됨
	m_tInfo.iMaxHp += HpIncreasePerLevel;
	m_tInfo.iHp += HpIncreasePerLevel;
	m_tInfo.iAttack += AttackIncreasePerLevel;

	CapstoneSFX::Play2D(this, ESFX::UI_LevelUp);

	if (IsValid(PlayerHpBar))
	{
		PlayerHpBar->SetHpValue(m_tInfo.iHp, m_tInfo.iMaxHp);
	}
}

void APy_Grey::HandleBossDefeated(FName MonsterID)
{
	// 보스 유언은 쓰러지는 연출 위에 겹쳐 나와야 하므로 입력을 막지 않는다
	if (StoryComponent && BossDeathDialogueLines.Num() > 0)
		StoryComponent->PlayDialogue(BossDeathDialogueLines, false);

	GetWorldTimerManager().SetTimer(EndingTimerHandle, this, &APy_Grey::PlayEpilogue, EndingDelay, false);
}

void APy_Grey::PlayEpilogue()
{
	if (StoryComponent)
	{
		StoryComponent->StopDialogue(); // 유언이 아직 떠 있으면 정리하고 시작

		if (EpilogueDialogueLines.Num() > 0 &&
			StoryComponent->PlayDialogue(EpilogueDialogueLines, true, FSimpleDelegate::CreateUObject(this, &APy_Grey::ShowEnding)))
			return;
	}
	ShowEnding(); // 대사 데이터가 없으면 기존 8단계 동작 그대로
}

void APy_Grey::ShowEnding()
{
	if (!IsValid(EndingWidgetClass))
		return;

	EndingWidget = CreateWidget<UUI_Ending>(GetWorld(), EndingWidgetClass);
	if (!IsValid(EndingWidget))
		return;

	int32 FinalLevel = ExperienceComponent ? ExperienceComponent->GetCurrentLevel() : 1;
	int32 FinalGold = InventoryComponent ? InventoryComponent->GetGold() : 0;

	EndingWidget->InitEnding(StartLevelName, FinalLevel, FinalGold);
	EndingWidget->AddToViewport();
	CapstoneSFX::Play2D(this, ESFX::UI_Ending);

	SetGameplayInputLocked(true);

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeUIOnly());
	}

	UGameplayStatics::SetGamePaused(GetWorld(), true);
}

// 데미지를 준 주체에서 몬스터를 찾는다. 몬스터 본체가 직접 때리는 경우(DamageCauser=몬스터),
// 몬스터에 부착된 무기가 때리는 경우(무기의 부착 부모), 몬스터가 시전한 이펙트가 때리는 경우(이펙트의 Instigator)를 모두 다룬다
static const AMonster* FindKillerMonster(AController* EventInstigator, AActor* DamageCauser)
{
	if (IsValid(DamageCauser))
	{
		if (const AMonster* Monster = Cast<AMonster>(DamageCauser))
			return Monster;

		if (const AMonster* Monster = Cast<AMonster>(DamageCauser->GetAttachParentActor()))
			return Monster;

		if (const AMonster* Monster = Cast<AMonster>(DamageCauser->GetInstigator()))
			return Monster;

		if (const AMonster* Monster = Cast<AMonster>(DamageCauser->GetOwner()))
			return Monster;
	}

	if (IsValid(EventInstigator))
		return Cast<AMonster>(EventInstigator->GetPawn());

	return nullptr;
}

void APy_Grey::HandleDeath(AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead)
		return;

	bIsDead = true;
	m_tInfo.iHp = 0; // 초과 피해로 음수가 남지 않게 (HP 비율을 읽는 적응형 AI 등을 위해)

	CapstoneSFX::Play2D(this, ESFX::Player_VoiceDeath);
	CapstoneSFX::Play2D(this, ESFX::UI_Death);

	const AMonster* Killer = FindKillerMonster(EventInstigator, DamageCauser);
	const FText KillerName = Killer ? Killer->GetDisplayName() : FText::GetEmpty();

	LOG(TEXT("플레이어 사망 (사망 원인=%s), [계속] 버튼을 누르면 %s에서 리스폰"), *KillerName.ToString(), *RespawnLevelName.ToString());

	// 인벤토리/대사창이 열린 채로 죽으면 마우스 커서와 입력 잠금이 남으므로 먼저 정리한다
	if (IsValid(InventoryWidget) && InventoryWidget->IsInViewport())
		ToggleInventory();

	if (IsValid(EquipmentWidget) && EquipmentWidget->IsInViewport())
	{
		EquipmentWidget->RemoveFromParent();
		RefreshPanelInputMode();
	}

	if (StoryComponent && StoryComponent->IsPlaying())
		StoryComponent->StopDialogue();

	// ACharBase::TakeDamage가 캡슐 콜리전을 끄므로, 이동 모드를 남겨두면 데스 모션 동안 바닥을 뚫고 떨어진다
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	EnableWeaponCollision(false);
	SetGameplayInputLocked(true);

	// 애님BP의 상태 머신과 무관하게 데스 모션을 확실히 재생한다. 단일 노드 재생으로 바꾸면 기존 애님 인스턴스는
	// 폐기되므로 캐시해둔 pGreyAnim을 비워 이후 입력 처리가 죽은 포인터를 건드리지 않게 한다
	if (IsValid(DeathAnimation))
	{
		pGreyAnim = nullptr;
		GetMesh()->PlayAnimation(DeathAnimation, false);
	}

	APlayerController* PC = Cast<APlayerController>(GetController());

	if (PC)
	{
		DisableInput(PC);

		if (IsValid(DeathWidgetClass))
		{
			DeathWidget = CreateWidget<UUI_Death>(PC, DeathWidgetClass);
		}
	}

	if (IsValid(DeathWidget))
	{
		DeathWidget->SetKillerName(KillerName);
		DeathWidget->OnContinue.AddUObject(this, &APy_Grey::RespawnAtVillage);

		// 체력바 등 다른 HUD 위젯 위에 덮이도록 높은 ZOrder로 띄운다
		DeathWidget->AddToViewport(100);

		// 게임 입력(공격/스킬/점프/대시 등)을 전부 끊고 UI만 받도록 하고, [계속] 버튼을 누를 수 있게 커서를 켠다
		PC->FlushPressedKeys();
		PC->bShowMouseCursor = true;

		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}
	else
	{
		// 사망 위젯을 만들 수 없는 경우(클래스 미지정 등)에는 버튼이 없으므로 일정 시간 뒤 자동으로 리스폰한다
		GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &APy_Grey::RespawnAtVillage, RespawnDelay, false);
	}
}

void APy_Grey::RespawnAtVillage()
{
	// 사망 UI를 띄우며 바꾼 UIOnly 입력 모드/마우스 커서/포커스는 뷰포트 쪽에 남아 OpenLevel 뒤 새 폰까지 이어진다.
	// 되돌리지 않으면 새 레벨에서 키 입력이 전혀 먹지 않으므로 레벨을 넘기기 전에 게임 입력으로 복원한다
	// (다른 UI들이 닫힐 때 FInputModeGameOnly로 되돌리는 것과 같은 처리)
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (IsValid(DeathWidget))
			DeathWidget->RemoveFromParent();

		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
		PC->FlushPressedKeys();
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (ULevelTransitionSubsystem* TransitionSubsystem = GI->GetSubsystem<ULevelTransitionSubsystem>())
		{
			// 태그를 비워두면 AGM_Main::ChoosePlayerStart가 게임을 처음 시작할 때와 같은 태그 없는 PlayerStart를 고른다
			TransitionSubsystem->SetPendingPlayerStartTag(NAME_None);

			// 체력은 최대치로 되살아난다. 인벤토리(아이템/골드)/경험치/공격력/무기 강화 단계는 그대로 넘기고, 퀘스트는 UQuestStateSubsystem에 이미 저장돼 있다
			m_tInfo.iHp = m_tInfo.iMaxHp;
			SaveStateForLevelTransition(*TransitionSubsystem);
		}
	}

	UGameplayStatics::OpenLevel(GetWorld(), RespawnLevelName);
}

void APy_Grey::SaveStateForLevelTransition(ULevelTransitionSubsystem& TransitionSubsystem) const
{
	// OpenLevel(하드 레벨 전환)은 플레이어 폰까지 파괴하므로, 폰이 들고 있던 인벤토리(아이템/골드)를
	// 게임 인스턴스 서브시스템에 잠깐 맡겨뒀다가 새 레벨의 새 폰이 BeginPlay에서 복원해간다
	if (InventoryComponent)
	{
		TransitionSubsystem.SetPendingInventory(InventoryComponent->Items, InventoryComponent->GetGold());
	}

	// 인벤토리와 마찬가지로 레벨/경험치도 폰과 함께 파괴되므로 넘겨서 새 폰이 복원하게 한다
	if (ExperienceComponent)
	{
		TransitionSubsystem.SetPendingExperience(ExperienceComponent->GetCurrentLevel(), ExperienceComponent->GetCurrentExp());
	}

	// 체력도 폰과 함께 파괴되므로 넘겨서 새 폰이 복원하게 한다 (그렇지 않으면 레벨 이동마다 풀피로 초기화됨)
	TransitionSubsystem.SetPendingHp(GetMaxHp(), GetCurrentHp());

	// 레벨업/무기 강화로 오른 공격력과 강화 단계 (스킬 A 버프는 타격 시점에 배율만 곱하므로 m_tInfo.iAttack에는 들어 있지 않다)
	TransitionSubsystem.SetPendingCombatStats(GetAttackPower(), WeaponLevel);

	// 끼고 있는 검/방패 (보너스는 위 공격력/체력에 이미 포함돼 있으므로 새 폰은 모습만 복원한다)
	if (EquipmentComponent)
	{
		TransitionSubsystem.SetPendingEquipment(EquipmentComponent->GetEquippedItems());
	}
}

int32 APy_Grey::GetNextEnhanceGoldCost() const
{
	return EnhanceGoldCost * (WeaponLevel + 1);
}

bool APy_Grey::CanEnhanceWeapon() const
{
	if (WeaponLevel >= MaxWeaponLevel)
		return false;

	if (!InventoryComponent)
		return false;

	if (InventoryComponent->GetItemCount(EnhanceMaterialID) < EnhanceMaterialCount)
		return false;

	if (InventoryComponent->GetGold() < GetNextEnhanceGoldCost())
		return false;

	return true;
}

bool APy_Grey::TryUseSkill(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= 3)
		return false;

	if (GetSkillCooldownRemaining(SlotIndex) > 0.f)
	{
		CapstoneSFX::Play2D(this, ESFX::UI_Deny, 0.6f); // 쿨다운 중
		return false;
	}

	SkillSlots[SlotIndex].LastUsedTime = GetWorld()->GetTimeSeconds();

	// 쿨다운이 끝나는 순간 작은 "준비됨" 소리
	GetWorldTimerManager().SetTimer(SkillReadyTimers[SlotIndex], FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (!bIsDead)
			CapstoneSFX::Play2D(this, ESFX::UI_SkillReady);
	}), FMath::Max(SkillSlots[SlotIndex].Cooldown, 0.1f), false);

	return true;
}

bool APy_Grey::TryEnhanceWeapon()
{
	if (!CanEnhanceWeapon())
		return false;

	int32 GoldCost = GetNextEnhanceGoldCost();

	InventoryComponent->RemoveItem(EnhanceMaterialID, EnhanceMaterialCount);
	InventoryComponent->TrySpendGold(GoldCost);

	AddAttackPower(AttackPerEnhance);
	++WeaponLevel;

	CapstoneSFX::Play2D(this, ESFX::UI_Enhance);

	OnWeaponEnhanced.Broadcast(WeaponLevel);

	LOG(TEXT("무기 강화 성공 : Level %d, 공격력 %d"), WeaponLevel, GetAttackPower());

	return true;
}

float APy_Grey::GetSkillCooldownRemaining(int32 SlotIndex) const
{
	if (SlotIndex < 0 || SlotIndex >= 3)
		return 0.f;

	float Elapsed = GetWorld()->GetTimeSeconds() - SkillSlots[SlotIndex].LastUsedTime;

	return FMath::Max(0.f, SkillSlots[SlotIndex].Cooldown - Elapsed);
}

float APy_Grey::GetSkillCooldown(int32 SlotIndex) const
{
	if (SlotIndex < 0 || SlotIndex >= 3)
		return 0.f;

	return SkillSlots[SlotIndex].Cooldown;
}

void APy_Grey::PlayHitStop(float DurationScale)
{
	// 종료 시각은 실시간(월드 시간이 아닌 시계) 기준이라 배속의 영향을 받지 않는다. 연타로 겹치면 더 늦은 쪽을 유지
	HitStopRealEnd = FMath::Max(HitStopRealEnd, FPlatformTime::Seconds() + HitStopDuration * DurationScale);

	// 충돌한 바로 이 프레임부터 멈추도록 다음 Tick을 기다리지 않고 즉시 적용한다
	UpdateTimeDilation();
}

void APy_Grey::UpdateTimeDilation()
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	const bool bHitStopActive = FPlatformTime::Seconds() < HitStopRealEnd;
	const float Desired = bHitStopActive ? HitStopTimeDilation : 1.f;

	if (FMath::IsNearlyEqual(Desired, AppliedTimeDilation))
		return;

	if (bHitStopActive)
	{
		// 시작 시점의 배속(보스 슬로모션 중일 수도 있음)을 기억해 두었다가 끝나면 그 값으로 돌려준다
		if (AppliedTimeDilation >= 1.f)
			DilationToRestore = UGameplayStatics::GetGlobalTimeDilation(World);

		UGameplayStatics::SetGlobalTimeDilation(World, Desired);
		AppliedTimeDilation = Desired;
		return;
	}

	// 다른 시스템(보스 사망 슬로모션 등)이 히트스톱 도중 배속을 바꿨다면 그쪽 값을 존중하고 건드리지 않는다
	if (FMath::IsNearlyEqual(UGameplayStatics::GetGlobalTimeDilation(World), AppliedTimeDilation))
		UGameplayStatics::SetGlobalTimeDilation(World, DilationToRestore);

	AppliedTimeDilation = 1.f;
}

void APy_Grey::ApplyHitImpact(AMonster* Monster, const FVector& HitDirection, float FeelScale, bool bFinisher, bool bKnockback)
{
	if (!IsValid(Monster))
		return;

	const bool bKilled = Monster->GetCurrentHp() <= 0;

	// 메시 반동은 결정타에도 넣는다 (죽는 순간에도 맞는 느낌이 나야 한다). 피격 몽타주가 없는 보스도 이걸로 맞는 느낌이 난다
	Monster->ApplyHitRecoil(HitDirection, HitRecoilDistance * FeelScale * (bFinisher ? FinisherRecoilScale : 1.f));

	// 죽은 몬스터는 데스 모션이 재생되고 캡슐 콜리전도 꺼져 있어 밀면 바닥을 뚫을 수 있으므로 밀지 않는다. 보스는 밀리지 않는다
	if (bKnockback && !bKilled && !Monster->bIsBoss)
	{
		const float KnockbackScale = bFinisher ? FinisherKnockbackScale : NormalKnockbackScale;
		Monster->LaunchCharacter(HitDirection * Monster->KnockbackStrength * KnockbackScale, true, false);
	}

	AddCameraPunch(HitDirection, FeelScale * (bFinisher ? 1.5f : 1.f));

	if (bKilled)
	{
		// 보스는 자체 사망 슬로모션이 있으므로 히트스톱까지 얹어 겹치지 않게 한다
		if (!Monster->bIsBoss)
			PlayHitStop(KillHitStopScale);

		PlayCameraShake(StrongCameraShakeClass, KillCameraShakeScale);
		PlayHitRumble(HitTakenRumbleIntensity, HitStopDuration * 3.f);
	}
}

void APy_Grey::AddCameraPunch(const FVector& WorldDirection, float Scale)
{
	if (!pCamera)
		return;

	CameraPunchFOVCurrent = FMath::Min(CameraPunchFOVCurrent + CameraPunchFOV * Scale, CameraPunchFOV * 2.f);

	// 카메라가 맞은 방향으로 파고들도록 월드 방향을 카메라 로컬로 바꿔 쌓는다
	const FVector LocalDirection = pCamera->GetComponentTransform().InverseTransformVectorNoScale(WorldDirection.GetSafeNormal());
	CameraPunchOffsetCurrent = (CameraPunchOffsetCurrent + LocalDirection * CameraPunchKick * Scale).GetClampedToMaxSize(CameraPunchKick * 2.f);
}

void APy_Grey::UpdateCameraPunch(float RealDelta)
{
	if (!pCamera)
		return;

	if (CameraPunchFOVCurrent < 0.01f && CameraPunchOffsetCurrent.IsNearlyZero(0.05f))
	{
		// 다 돌아왔으면 기준값으로 정확히 되돌려 두고, 이후로는 카메라를 건드리지 않는다
		if (bCameraPunchApplied)
		{
			CameraPunchFOVCurrent = 0.f;
			CameraPunchOffsetCurrent = FVector::ZeroVector;
			pCamera->SetFieldOfView(BaseCameraFOV);
			pCamera->SetRelativeLocation(BaseCameraRelativeLocation);
			bCameraPunchApplied = false;
		}

		return;
	}

	// 맞은 프레임에는 가장 크게 튄 상태를 그대로 보여주고, 그 뒤부터 지수적으로 감쇠시킨다
	pCamera->SetFieldOfView(BaseCameraFOV - CameraPunchFOVCurrent);
	pCamera->SetRelativeLocation(BaseCameraRelativeLocation + CameraPunchOffsetCurrent);
	bCameraPunchApplied = true;

	const float Alpha = FMath::Clamp(RealDelta * CameraPunchReturnSpeed, 0.f, 1.f);
	CameraPunchFOVCurrent = FMath::Lerp(CameraPunchFOVCurrent, 0.f, Alpha);
	CameraPunchOffsetCurrent = FMath::Lerp(CameraPunchOffsetCurrent, FVector::ZeroVector, Alpha);
}

void APy_Grey::ApplyAttackAssist()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();

	if (bIsDead || !LockOnComponent || !Movement || Movement->IsFalling())
		return;

	AMonster* Target = LockOnComponent->FindAssistTarget(AttackAssistRange, AttackAssistHalfAngle);

	if (!IsValid(Target))
		return;

	FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;

	const float Distance = ToTarget.Size();

	if (Distance < 1.f)
		return;

	// 캡슐 표면 사이의 거리로 재야 덩치 큰 보스 앞에서도 알맞은 위치에서 멈춘다
	const float Gap = Distance - GetCapsuleComponent()->GetScaledCapsuleRadius() - Target->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float Travel = FMath::Min(Gap - AttackAssistStopGap, AttackAssistMaxTravel);

	// 이미 충분히 가까우면 파고들지 않는다 (콤보 후반부나 붙어 싸울 때 밀고 들어가 겹치는 일을 막는다)
	if (Travel < 10.f)
		return;

	LaunchCharacter(ToTarget / Distance * ComputeLaunchSpeedForTravel(*Movement, Travel), true, false);
}

void APy_Grey::PlayCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale)
{
	if (!ShakeClass)
		return;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
		PC->ClientStartCameraShake(ShakeClass, Scale);
}

void APy_Grey::PlayHitRumble(float Intensity, float Duration)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
		return;

	PC->PlayDynamicForceFeedback(Intensity, Duration, false, false, true, true, EDynamicForceFeedbackAction::Start);
}

void APy_Grey::EnableWeaponCollision(bool bEnable)
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

void APy_Grey::WepaonBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	FDamageEvent	DamageEvent;

	FVector vStart = GetMesh()->GetSocketLocation(TEXT("FX_Sword_Bottom"));
	FVector vEnd = GetMesh()->GetSocketLocation(TEXT("FX_Sword_Top"));

	FVector vDir = vEnd - vStart;
	vDir.Normalize();

	FCollisionQueryParams tColParams(NAME_None, false, this);
	TArray<FHitResult> HitArray;
	bool bCollision = GetWorld()->SweepMultiByProfile(HitArray, vStart, vEnd, vDir.ToOrientationQuat(), TEXT("PlayerAttack"), FCollisionShape::MakeBox(FVector(7.5f, 2.5f, 60.f)),
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
		float fDamage = OtherActor->TakeDamage(RollAttackDamage(), DamageEvent, GetController(), this);

		// 콤보가 이어질수록 카메라 셰이크/히트스탑이 점점 강해져서 연타에 무게감이 붙도록
		int32 CurrentCombo = IsValid(pGreyAnim) ? pGreyAnim->GetCurrentCombo() : 0;
		float ComboScale = FMath::Clamp(1.f + CurrentCombo * ComboFeelScalePerHit, 1.f, MaxComboFeelScale);

		PlayHitStop(ComboScale);
		PlayCameraShake(WeakCameraShakeClass, ComboScale);
		PlayHitRumble(HitLandedRumbleIntensity * ComboScale, HitStopDuration * 2.f);

		// 몬스터가 뒤로 밀리고 메시가 튕기며 카메라가 파고드는 임팩트. 마지막 콤보 타는 훨씬 크게 들어간다
		if (AMonster* HitMonster = Cast<AMonster>(OtherActor))
		{
			const FVector HitDirection = (HitMonster->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();

			// 검이 몸을 때리는 소리 (갑옷/바위는 금속성). 마무리 타는 더 크게
			const bool bFinisherHit = CurrentCombo >= FinisherComboIndex;
			CapstoneSFX::PlayAt(this, HitMonster->GetPlayerHitSFX(), vImpactPoint.IsZero() ? HitMonster->GetActorLocation() : vImpactPoint, bFinisherHit ? 1.25f : 1.f);

			ApplyHitImpact(HitMonster, HitDirection, ComboScale, bFinisherHit, true);
		}

		// 폭발 버프 중이 아니어도 매 타격마다 항상 나가는 기본 임팩트 이펙트(스파크+타격음)
		SpawnAttachedHitEffect(NormalHitEffectClass, OtherActor, vImpactPoint, vImpactNormal);

		if (bIsExplosiveBuffActive)
		{
			SpawnAttachedHitEffect(HitEffectClass, OtherActor, vImpactPoint, vImpactNormal);
		}
	}

#if ENABLE_DRAW_DEBUG
	FVector vCenter = (vStart + vEnd) / 2.f;
	FColor	DrawColor = HitArray.Num() > 0 ? FColor::Red : FColor::Green;

	DrawDebugBox(GetWorld(), vCenter, FVector(7.5f, 2.5f, 60.f), vDir.ToOrientationQuat(), DrawColor, false, 1.5f);
#endif
}

void APy_Grey::WeaponEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{

}

void APy_Grey::SpawnAttachedHitEffect(TSubclassOf<AEffectDestroy> EffectClass, AActor* Target, const FVector& ImpactPoint, const FVector& ImpactNormal)
{
	if (!EffectClass)
		return;

	FActorSpawnParameters params;
	params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// 타격 지점에 스폰하되, 몬스터의 루트 컴포넌트에 붙여서 넉백 등으로 몬스터가 움직여도
	// 이펙트가 허공에 남지 않고 몬스터를 따라가도록 함
	AEffectDestroy* Effect = GetWorld()->SpawnActor<AEffectDestroy>(
		EffectClass,
		ImpactPoint,
		ImpactNormal.Rotation(),
		params
	);

	if (!IsValid(Effect))
		return;

	if (IsValid(Target) && Target->GetRootComponent())
	{
		USceneComponent* AttachComp = Target->GetRootComponent();
		FName SocketName = NAME_None;

		// 맞은 대상이 몬스터고 BP에 소켓 이름이 지정돼 있으면 그 소켓 위치에 정확히 스폰
		if (AMonster* HitMonster = Cast<AMonster>(Target))
		{
			if (HitMonster->HitEffectSocketName != NAME_None)
			{
				if (USkeletalMeshComponent* HitMesh = HitMonster->GetMesh())
				{
					if (HitMesh->DoesSocketExist(HitMonster->HitEffectSocketName))
					{
						AttachComp = HitMesh;
						SocketName = HitMonster->HitEffectSocketName;
					}
				}
			}
		}

		if (SocketName != NAME_None)
		{
			// 소켓 위치/회전에 딱 맞춰 붙임 (계산된 임팩트 지점 대신 소켓을 사용)
			Effect->AttachToComponent(AttachComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
		}
		else
		{
			// 소켓이 없으면 기존처럼 실제 타격 지점에서 시작해 몬스터를 따라가도록 붙임
			Effect->AttachToComponent(AttachComp, FAttachmentTransformRules::KeepWorldTransform);
		}
	}

	Effect->InitializeEffect();
}

UNiagaraComponent* APy_Grey::SpawnPowerSlashFX(UNiagaraSystem* System, FName SocketName)
{
	if (!IsValid(System))
		return nullptr;

	// 스킬이 끝나면 StopPowerSlashFX가 Deactivate만 하고, 남은 파티클이 다 사라진 뒤에 컴포넌트가 스스로 정리되도록 AutoDestroy로 스폰한다
	return UNiagaraFunctionLibrary::SpawnSystemAttached(
		System,
		GetMesh(),
		SocketName,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		FVector(PowerSlashFXScale),
		EAttachLocation::KeepRelativeOffset,
		true,
		ENCPoolMethod::None
	);
}

void APy_Grey::StopPowerSlashFX(TWeakObjectPtr<UNiagaraComponent>& FXComp)
{
	if (UNiagaraComponent* Comp = FXComp.Get())
	{
		Comp->Deactivate();
	}

	FXComp.Reset();
}

void APy_Grey::BeginPowerSlash()
{
	// 이전 스윙이 남아 있으면(쿨다운을 짧게 조정한 경우 등) 먼저 정리한다
	EndPowerSlash();

	PowerSlashHitActors.Reset();

	// 검 오라는 스킬을 쓰는 순간부터 붙어서, 검을 치켜드는 준비 동작 때부터 검이 달아오르는 것처럼 보이게 한다
	PowerSlashSwordFXComp = SpawnPowerSlashFX(PowerSlashSwordFX, TEXT("FX_Sword_Mid"));

	GetWorldTimerManager().SetTimer(PowerSlashSwingStartTimerHandle, this, &APy_Grey::BeginPowerSlashSwing, FMath::Max(PowerSlashHitDelay, KINDA_SMALL_NUMBER), false);

	LOG(TEXT("스킬 C 강력 가로베기 시작 : 공격력 x%.2f, 판정 %.2f초 뒤 %.2f초간"), PowerSlashDamageMultiplier, PowerSlashHitDelay, PowerSlashHitDuration);
}

void APy_Grey::BeginPowerSlashSwing()
{
	bPowerSlashSwinging = true;
	LastPowerSlashBladeMid = GetMesh()->GetSocketLocation(TEXT("FX_Sword_Mid"));
	LastPowerSlashBladeTop = GetMesh()->GetSocketLocation(TEXT("FX_Sword_Top"));

	PowerSlashTrailFXComp = SpawnPowerSlashFX(PowerSlashTrailFX, TEXT("FX_Sword_Top"));

	GetWorldTimerManager().SetTimer(PowerSlashSwingEndTimerHandle, this, &APy_Grey::EndPowerSlashSwing, FMath::Max(PowerSlashHitDuration, KINDA_SMALL_NUMBER), false);
}

void APy_Grey::EndPowerSlashSwing()
{
	bPowerSlashSwinging = false;

	StopPowerSlashFX(PowerSlashTrailFXComp);
	StopPowerSlashFX(PowerSlashSwordFXComp);

	LOG(TEXT("스킬 C 강력 가로베기 종료 : 명중 %d마리"), PowerSlashHitActors.Num());
}

void APy_Grey::EndPowerSlash()
{
	GetWorldTimerManager().ClearTimer(PowerSlashSwingStartTimerHandle);
	GetWorldTimerManager().ClearTimer(PowerSlashSwingEndTimerHandle);

	bPowerSlashSwinging = false;

	StopPowerSlashFX(PowerSlashTrailFXComp);
	StopPowerSlashFX(PowerSlashSwordFXComp);
}

void APy_Grey::UpdatePowerSlashHit()
{
	const FVector BladeMid = GetMesh()->GetSocketLocation(TEXT("FX_Sword_Mid"));
	const FVector BladeTop = GetMesh()->GetSocketLocation(TEXT("FX_Sword_Top"));

	// 이전 프레임의 검 위치와 지금 위치 사이를 촘촘히 훑어서, 검이 빠르게 휘둘러지거나 프레임이 떨어져도 몬스터를 건너뛰지 않게 한다
	const float TravelDistance = FMath::Max(FVector::Dist(LastPowerSlashBladeMid, BladeMid), FVector::Dist(LastPowerSlashBladeTop, BladeTop));
	const int32 Steps = FMath::Clamp(FMath::CeilToInt(TravelDistance / FMath::Max(PowerSlashHitRadius * 0.5f, 1.f)), 1, 8);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PowerSlash), false, this);

	// ECC_GameTraceChannel3 = 프로젝트 설정의 "Monster" 오브젝트 타입 (몬스터 캡슐이 쓰는 채널)
	const FCollisionObjectQueryParams ObjectParams(ECC_GameTraceChannel3);
	const FCollisionShape HitSphere = FCollisionShape::MakeSphere(PowerSlashHitRadius);

	bool bAnyNewHit = false;


	// 검 중간과 검 끝 두 지점에서 판정해서, 검이 닿는 범위 전체(손 앞 ~ 검 끝 너머)를 넓은 호로 커버한다
	for (int32 Step = 1; Step <= Steps; ++Step)
	{
		const float Alpha = static_cast<float>(Step) / Steps;

		for (int32 Sample = 0; Sample < 2; ++Sample)
		{
			const FVector Center = Sample == 0
				? FMath::Lerp(LastPowerSlashBladeMid, BladeMid, Alpha)
				: FMath::Lerp(LastPowerSlashBladeTop, BladeTop, Alpha);

			TArray<FOverlapResult> Overlaps;
			GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, ObjectParams, HitSphere, QueryParams);

			for (const FOverlapResult& Overlap : Overlaps)
			{
				AMonster* Monster = Cast<AMonster>(Overlap.GetActor());

				if (!IsValid(Monster) || Monster->GetCurrentHp() <= 0 || PowerSlashHitActors.Contains(Monster))
					continue;

				const FVector ToMonster = Monster->GetActorLocation() - GetActorLocation();

				if (ToMonster.Size2D() > PowerSlashMaxRange)
					continue;

				PowerSlashHitActors.Add(Monster);
				bAnyNewHit = true;

				const float Damage = RollAttackDamage() * PowerSlashDamageMultiplier;
				const FVector HitDir = ToMonster.GetSafeNormal2D();

				// AMonster::TakeDamage는 protected라서 AActor의 공개 진입점으로 호출한다 (일반 공격과 동일한 경로)
				FDamageEvent DamageEvent;
				static_cast<AActor*>(Monster)->TakeDamage(Damage, DamageEvent, GetController(), this);

				LOG(TEXT("스킬 C 명중 : %s 피해 %.0f -> HP %d/%d"), *Monster->GetName(), Damage, Monster->GetCurrentHp(), Monster->GetMaxHp());

				const FVector ImpactPoint = Monster->GetActorLocation();
				const FVector ImpactNormal = -HitDir;

				if (IsValid(PowerSlashHitFX))
				{
					UNiagaraComponent* HitFXComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), PowerSlashHitFX, ImpactPoint, ImpactNormal.Rotation(), FVector(PowerSlashHitFXScale));

					if (IsValid(HitFXComp))
					{
						// 루프형 이펙트라서 정해진 시간 뒤에 끄면, 남은 파티클이 사라진 다음 컴포넌트가 스스로 정리된다
						FTimerHandle HitFXTimerHandle;
						TWeakObjectPtr<UNiagaraComponent> WeakHitFX = HitFXComp;
						GetWorldTimerManager().SetTimer(HitFXTimerHandle, FTimerDelegate::CreateLambda([WeakHitFX]() { if (WeakHitFX.IsValid()) WeakHitFX->Deactivate(); }), PowerSlashHitFXDuration, false);
					}
				}

				SpawnAttachedHitEffect(NormalHitEffectClass, Monster, ImpactPoint, ImpactNormal);

				if (bIsExplosiveBuffActive)
				{
					SpawnAttachedHitEffect(HitEffectClass, Monster, ImpactPoint, ImpactNormal);
				}

				CapstoneSFX::PlayAt(this, ESFX::Player_SkillC_Impact, ImpactPoint);
				CapstoneSFX::PlayAt(this, Monster->GetPlayerHitSFX(), ImpactPoint, 1.25f);

				// 스킬 C는 마무리 타급 임팩트(반동/카메라 펀치/처치 연출). 넉백은 아래에서 전용 배율로 따로 처리한다
				ApplyHitImpact(Monster, HitDir, 1.f, true, false);

				// 살아남았다면 검이 휘둘러진 방향으로 크게 밀어낸다. 보스는 밀리지 않는다
				if (Monster->GetCurrentHp() > 0 && !Monster->bIsBoss)
				{
					Monster->LaunchCharacter(HitDir * Monster->KnockbackStrength * PowerSlashKnockbackScale + FVector(0.f, 0.f, 120.f), true, false);
				}
			}
		}
	}

	LastPowerSlashBladeMid = BladeMid;
	LastPowerSlashBladeTop = BladeTop;

	// 여러 마리를 동시에 벨 때도 타격감 연출은 한 번만 (히트스탑이 마릿수만큼 연장되면 오히려 답답해진다)
	if (bAnyNewHit)
	{
		PlayHitStop(PowerSlashFeelScale);
		PlayCameraShake(WeakCameraShakeClass, PowerSlashFeelScale);
		PlayHitRumble(HitLandedRumbleIntensity * PowerSlashFeelScale, HitStopDuration * 2.f);
	}
}

float APy_Grey::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 이미 사망했으면 리스폰을 기다리는 동안 들어오는 추가 피해(중복 사망 처리)를 무시한다
	if (bInvincible || bIsDead)
		return 0.f;

	// 보호막이 체력보다 먼저 피해를 흡수한다. 남은 피해만 체력에 들어가고, 보호막이 바닥나면 VFX까지 함께 정리
	if (CurrentShield > 0.f && DamageAmount > 0.f)
	{
		const float Absorbed = FMath::Min(CurrentShield, DamageAmount);
		CurrentShield -= Absorbed;
		DamageAmount -= Absorbed;

		if (CurrentShield <= 0.f)
		{
			EndShield();
			CapstoneSFX::Play2D(this, ESFX::Player_ShieldBreak);
		}
		else
		{
			RefreshShieldUI();
			CapstoneSFX::Play2D(this, ESFX::Player_ShieldHit);
		}

		// 보호막이 전부 막아낸 타격은 체력이 깎이지 않으므로 피격 연출(히트스톱/카메라 셰이크/진동)도 생략
		if (DamageAmount <= 0.f)
			return 0.f;
	}

	float fDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (IsValid(PlayerHpBar))
		PlayerHpBar->SetHpValue(m_tInfo.iHp, m_tInfo.iMaxHp);

	// 몸에 맞는 소리 + (살아 있으면) 신음. 최대 체력의 15% 이상을 한 번에 맞으면 큰 신음
	CapstoneSFX::Play2D(this, ESFX::Player_Hurt);

	if (m_tInfo.iHp > 0)
		CapstoneSFX::Play2D(this, fDamage >= m_tInfo.iMaxHp * 0.15f ? ESFX::Player_VoicePainHeavy : ESFX::Player_VoicePain);

	PlayHitStop();
	PlayCameraShake(StrongCameraShakeClass);
	PlayHitRumble(HitTakenRumbleIntensity, HitStopDuration * 3.f);

	if (m_tInfo.iHp <= 0)
	{
		HandleDeath(EventInstigator, DamageCauser);
	}

	return fDamage;
}
