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
#include "NPC_Base.h"
#include "Engine/DamageEvents.h"
#include "DrawDebugHelpers.h"
#include "NiagaraFunctionLibrary.h"

APy_Grey::APy_Grey()
{
	PrimaryActorTick.bCanEverTick = true;

	m_tInfo.iMaxHp = 500;
	m_tInfo.iHp = 500;
	m_tInfo.iAttack = 10;

	pArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Arm"));
	pCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));

	pArm->SetupAttachment(GetMesh());
	pCamera->SetupAttachment(pArm);

	pArm->bUsePawnControlRotation = true;
	
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

}


void APy_Grey::BeginPlay()
{
	Super::BeginPlay();

	pGreyAnim = Cast<UPy_GreyAnim>(GetMesh()->GetAnimInstance());

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Player"));
	WeaponBox->SetCollisionProfileName(TEXT("PlayerAttack"));

	WeaponBox->OnComponentBeginOverlap.AddDynamic(this, &APy_Grey::WepaonBeginOverlap);
	WeaponBox->OnComponentEndOverlap.AddDynamic(this, &APy_Grey::WeaponEndOverlap);

	WeaponBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponBox->SetBoxExtent({ 7.5f, 2.5f, 60.f});

	if (IsValid(PlayerHpBarClass))
	{
		PlayerHpBar = CreateWidget<UUI_HpBar>(GetWorld(), PlayerHpBarClass);

		if (IsValid(PlayerHpBar))
		{
			PlayerHpBar->AddToViewport();

			float fPercent = m_tInfo.iHp / m_tInfo.iMaxHp;
			PlayerHpBar->SetHp(fPercent);
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

	if (IsValid(pGreyAnim))
		pGreyAnim->SkillSlotA();
}

void APy_Grey::SkillSlotB()
{
	if (bGameplayInputLocked)
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

	if (IsValid(pGreyAnim))
		pGreyAnim->SkillSlotC();
}

void APy_Grey::OnInteractKeyPressed()
{
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
	Heal(Data->HealAmount);

	LOG(TEXT("UsePotionItem(%s) 사용 : HealAmount=%d, HP %d -> %d / %d"), *ItemID.ToString(), Data->HealAmount, HpBeforeHeal, m_tInfo.iHp, m_tInfo.iMaxHp);

	if (IsValid(PlayerHpBar))
	{
		PlayerHpBar->SetHp(m_tInfo.iHp / static_cast<float>(m_tInfo.iMaxHp));
	}
}

void APy_Grey::RollOrDash()
{
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
	LaunchCharacter(InputDirection * 8000.f, true, true);

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
		PlayerHpBar->SetHp(m_tInfo.iHp / static_cast<float>(m_tInfo.iMaxHp));
	}
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

		float fDamage = OtherActor->TakeDamage(m_tInfo.iAttack, DamageEvent, GetController(), this);

		if (bIsExplosiveBuffActive && HitEffectClass)
		{
			AEffectDestroy* Effect = GetWorld()->SpawnActor<AEffectDestroy>(
				HitEffectClass,          
				vImpactPoint,            
				vImpactNormal.Rotation(),
				params
			);

			if (IsValid(Effect))
			{
				Effect->InitializeEffect();
			}
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
	float fDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (IsValid(PlayerHpBar))
		PlayerHpBar->SetHp(m_tInfo.iHp / static_cast<float>(m_tInfo.iMaxHp));

	return fDamage;
}
