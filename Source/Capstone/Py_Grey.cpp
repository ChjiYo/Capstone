#include "Py_Grey.h"
#include "Py_GreyAnim.h"
#include "EffectDestroy.h"
#include "ItemBase.h"
#include "InventoryComponent.h"
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
#include "Animation/AnimSequence.h"
#include "DrawDebugHelpers.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraShakeBase.h"
#include "CombatPatternSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "LevelTransitionSubsystem.h"

APy_Grey::FOnPlayerDodge APy_Grey::OnPlayerDodge;

APy_Grey::FOnWeaponEnhanced APy_Grey::OnWeaponEnhanced;

APy_Grey::APy_Grey()
{
	PrimaryActorTick.bCanEverTick = true;

	m_tInfo.iMaxHp = 200;
	m_tInfo.iHp = 200;
	m_tInfo.iAttack = 10;

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

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	QuestComponent = CreateDefaultSubobject<UQuestComponent>(TEXT("QuestComponent"));
	ExperienceComponent = CreateDefaultSubobject<UExperienceComponent>(TEXT("ExperienceComponent"));
	StoryComponent = CreateDefaultSubobject<UStoryComponent>(TEXT("StoryComponent"));

	DeathWidgetClass = UUI_Death::StaticClass();

	static ConstructorHelpers::FObjectFinder<UAnimSequence> DeathAnimAsset(TEXT("/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Death.Death"));

	if (DeathAnimAsset.Succeeded())
		DeathAnimation = DeathAnimAsset.Object;
}


void APy_Grey::BeginPlay()
{
	Super::BeginPlay();

	pGreyAnim = Cast<UPy_GreyAnim>(GetMesh()->GetAnimInstance());

	DefaultBrakingDecelerationFalling = GetCharacterMovement()->BrakingDecelerationFalling;
	DefaultFallingLateralFriction = GetCharacterMovement()->FallingLateralFriction;

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
		}
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

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackBuffTimerHandle);
		World->GetTimerManager().ClearTimer(ShieldTimerHandle);
		World->GetTimerManager().ClearTimer(RespawnTimerHandle);
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
	PlayerInputComponent->BindAction(TEXT("UsePotion"), EInputEvent::IE_Pressed, this, &APy_Grey::UsePotion);

	PlayerInputComponent->BindAction(TEXT("RollOrDash"), EInputEvent::IE_Pressed, this, &APy_Grey::RollOrDash);
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
	AddControllerYawInput(_fScale);
}

void APy_Grey::RotationX(float _fScale)
{
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

void APy_Grey::SkillSlotB()
{
	if (bGameplayInputLocked)
		return;

	if (!TryUseSkill(1))
		return;

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
		CurrItem->PickupItem(this);
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

	APlayerController* PC = Cast<APlayerController>(GetController());

	if (InventoryWidget->IsInViewport())
	{
		InventoryWidget->RemoveFromParent();
		SetGameplayInputLocked(false);

		if (PC)
		{
			PC->bShowMouseCursor = false;
			PC->SetInputMode(FInputModeGameOnly());
			// UI가 떠 있는 동안 마우스 클릭(NormalAttack 키) 등이 눌린 채로 걸려 있을 수 있어 초기화
			PC->FlushPressedKeys();
		}
	}
	else
	{
		InventoryWidget->AddToViewport();
		SetGameplayInputLocked(true);

		if (InventoryComponent)
		{
			InventoryWidget->RefreshInventoryUI(InventoryComponent);
		}

		if (PC)
		{
			PC->bShowMouseCursor = true;
			PC->SetInputMode(FInputModeGameAndUI());
		}
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

	const AMonster* Killer = FindKillerMonster(EventInstigator, DamageCauser);
	const FText KillerName = Killer ? Killer->GetDisplayName() : FText::GetEmpty();

	LOG(TEXT("플레이어 사망 (사망 원인=%s), [계속] 버튼을 누르면 %s에서 리스폰"), *KillerName.ToString(), *RespawnLevelName.ToString());

	// 인벤토리/대사창이 열린 채로 죽으면 마우스 커서와 입력 잠금이 남으므로 먼저 정리한다
	if (IsValid(InventoryWidget) && InventoryWidget->IsInViewport())
		ToggleInventory();

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
		return false;

	SkillSlots[SlotIndex].LastUsedTime = GetWorld()->GetTimeSeconds();

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
	UWorld* World = GetWorld();
	if (!World)
		return;

	UGameplayStatics::SetGlobalTimeDilation(World, HitStopTimeDilation);

	// dilation이 적용된 뒤라 SetTimer의 실시간 길이가 짧아지므로, 노출된 실시간(초) 값에 dilation을 곱해 보정
	World->GetTimerManager().SetTimer(HitStopTimerHandle, this, &APy_Grey::EndHitStop, HitStopDuration * DurationScale * HitStopTimeDilation, false);
}

void APy_Grey::EndHitStop()
{
	UWorld* World = GetWorld();
	if (World)
		UGameplayStatics::SetGlobalTimeDilation(World, 1.f);
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
		FActorSpawnParameters params;
		params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		float fDamage = OtherActor->TakeDamage(GetEffectiveAttackPower(), DamageEvent, GetController(), this);

		// 콤보가 이어질수록 카메라 셰이크/히트스탑이 점점 강해져서 연타에 무게감이 붙도록
		int32 CurrentCombo = IsValid(pGreyAnim) ? pGreyAnim->GetCurrentCombo() : 0;
		float ComboScale = FMath::Clamp(1.f + CurrentCombo * ComboFeelScalePerHit, 1.f, MaxComboFeelScale);

		PlayHitStop(ComboScale);
		PlayCameraShake(WeakCameraShakeClass, ComboScale);
		PlayHitRumble(HitLandedRumbleIntensity * ComboScale, HitStopDuration * 2.f);

		// 타격 지점에 스폰하되, 몬스터의 루트 컴포넌트에 붙여서 넉백 등으로 몬스터가 움직여도
		// 이펙트가 허공에 남지 않고 몬스터를 따라가도록 함
		auto SpawnAttachedHitEffect = [&](TSubclassOf<AEffectDestroy> EffectClass)
		{
			if (!EffectClass)
				return;

			AEffectDestroy* Effect = GetWorld()->SpawnActor<AEffectDestroy>(
				EffectClass,
				vImpactPoint,
				vImpactNormal.Rotation(),
				params
			);

			if (!IsValid(Effect))
				return;

			if (IsValid(OtherActor) && OtherActor->GetRootComponent())
			{
				USceneComponent* AttachComp = OtherActor->GetRootComponent();
				FName SocketName = NAME_None;

				// 맞은 대상이 몬스터고 BP에 소켓 이름이 지정돼 있으면 그 소켓 위치에 정확히 스폰
				if (AMonster* HitMonster = Cast<AMonster>(OtherActor))
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
		};

		// 폭발 버프 중이 아니어도 매 타격마다 항상 나가는 기본 임팩트 이펙트(스파크+타격음)
		SpawnAttachedHitEffect(NormalHitEffectClass);

		if (bIsExplosiveBuffActive)
		{
			SpawnAttachedHitEffect(HitEffectClass);
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
			EndShield();
		else
			RefreshShieldUI();

		// 보호막이 전부 막아낸 타격은 체력이 깎이지 않으므로 피격 연출(히트스톱/카메라 셰이크/진동)도 생략
		if (DamageAmount <= 0.f)
			return 0.f;
	}

	float fDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (IsValid(PlayerHpBar))
		PlayerHpBar->SetHpValue(m_tInfo.iHp, m_tInfo.iMaxHp);

	PlayHitStop();
	PlayCameraShake(StrongCameraShakeClass);
	PlayHitRumble(HitTakenRumbleIntensity, HitStopDuration * 3.f);

	if (m_tInfo.iHp <= 0)
	{
		HandleDeath(EventInstigator, DamageCauser);
	}

	return fDamage;
}
