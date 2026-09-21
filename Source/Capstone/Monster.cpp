#include "Monster.h"
#include "MonsterAnim.h"
#include "MonsterAIController.h"
#include "Components/WidgetComponent.h"
#include "Components/PrimitiveComponent.h"
#include "UI_HpBar.h"
#include "UI_DamageNumber.h"
#include "Kismet/GameplayStatics.h"
#include "ItemBase.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

FOnMonsterKilled AMonster::OnMonsterKilled;
FOnBossDefeated AMonster::OnBossDefeated;

AMonster::AMonster()
{
	PrimaryActorTick.bCanEverTick = true;
	pSpawnPoint = nullptr;

	fTraceRange = 1000.f;
	fAttackDist = 150.f;

	HpBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HpBar"));
	HpBarComponent->SetupAttachment(GetMesh());
	HpBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HpBarComponent->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	// 탐지 범위 밖에서는 숨겨두고, Tick에서 AI가 플레이어를 감지했을 때만(UBTService_Detect -> SetTarget) 보이게 한다
	HpBarComponent->SetVisibility(false);

	static ConstructorHelpers::FClassFinder<UUserWidget> HpBarWidgetClass(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/UE_Asset/UI/WBP_UI_Mon_HpBar.WBP_UI_Mon_HpBar_C'"));

	if (HpBarWidgetClass.Succeeded())
		HpBarComponent->SetWidgetClass(HpBarWidgetClass.Class);

	bHit = false;

	DamageNumberWidgetClass = UUI_DamageNumber::StaticClass();
}

AMonster::~AMonster()
{
	OnAttackEnd.Clear();
	AttackEndHandle.Reset(0);
}

void AMonster::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// SpawnPoint에 대한 재생성 통지는 반드시 EndPlay에서 처리해야 한다. 이전에는 이 로직이
	// ~AMonster() 소멸자에 있었는데, UObject의 C++ 소멸자는 가비지 컬렉션 타이밍에 임의로
	// (죽은 직후가 아니라 한참 뒤에, 또는 레벨 전환/PIE 종료로 SpawnPoint 자신도 같이 파괴되는
	// 도중에) 호출되기 때문에 pSpawnPoint가 이미 해제된 상태를 가리킬 수 있었다. 그 결과
	// (1) "체크박스를 꺼도 확률적으로 몬스터가 다시 생성되는" 것처럼 보이는 현상과
	// (2) pSpawnPoint->Respawn() 호출 시 EXCEPTION_ACCESS_VIOLATION 크래시가 둘 다 발생했다.
	// EndPlayReason이 Destroyed(실제 처치로 인한 Destroy() 호출)일 때만 재생성을 통지해서,
	// 레벨 전환(OpenLevel)이나 PIE 종료로 월드 전체가 정리되는 중에는 재생성이 일어나지 않게 한다.
	if (EndPlayReason == EEndPlayReason::Destroyed && IsValid(pSpawnPoint))
	{
		pSpawnPoint->Respawn();
	}

	Super::EndPlay(EndPlayReason);
}

FText AMonster::GetDisplayName() const
{
	if (!DisplayName.IsEmpty())
		return DisplayName;

	return FText::FromName(MonsterID);
}

void AMonster::ClearAttackEndDelegate()
{
	OnAttackEnd.Clear();
}

void AMonster::BeginPlay()
{
	Super::BeginPlay();

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Monster"));
	GetCapsuleComponent()->SetNotifyRigidBodyCollision(true);


	HpBarWidget = Cast<UUI_HpBar>(HpBarComponent->GetUserWidgetObject());

	if (IsValid(HpBarWidget))
		HpBarWidget->SetHp(m_tInfo.iHp / static_cast<float>(m_tInfo.iMaxHp));

	// 생성자/블루프린트가 정해 둔 메시 위치를 반동 연출의 기준점으로 기억해 둔다
	if (GetMesh())
		MeshBaseRelativeLocation = GetMesh()->GetRelativeLocation();

	LastFootstepLocation = GetActorLocation();

	if (USkeletalMeshComponent* MonsterMesh = GetMesh())
	{
		HitFlashMaterials.Reset();

		const int32 NumMaterials = MonsterMesh->GetNumMaterials();
		for (int32 i = 0; i < NumMaterials; ++i)
		{
			if (UMaterialInstanceDynamic* DynMat = MonsterMesh->CreateAndSetMaterialInstanceDynamic(i))
			{
				HitFlashMaterials.Add(DynMat);

				// [TEMP DEBUG] 이 머티리얼 슬롯이 실제로 어떤 머티리얼을 쓰고 있고, 그 안에
				// FlashAmount/DissolveAmount 스칼라 파라미터가 실제로 존재하는지 확인용 로그.
				// 디졸브가 안 보인다면 여기서 DissolveAmount가 "찾음: 아니오"로 나오는지부터 확인.
				TArray<FMaterialParameterInfo> ParamInfos;
				TArray<FGuid> ParamIds;
				DynMat->GetAllScalarParameterInfo(ParamInfos, ParamIds);

				bool bHasFlash = false;
				bool bHasDissolve = false;
				FString AllNames;
				for (const FMaterialParameterInfo& Info : ParamInfos)
				{
					AllNames += Info.Name.ToString() + TEXT(", ");
					if (Info.Name == HitFlashParamName) bHasFlash = true;
					if (Info.Name == DissolveParamName) bHasDissolve = true;
				}

				LOG(TEXT("[슬롯 %d] 머티리얼=%s / 스칼라 파라미터 목록=[%s] / FlashAmount 찾음=%s / DissolveAmount 찾음=%s / BlendMode=%d"),
					i,
					DynMat->GetMaterial() ? *DynMat->GetMaterial()->GetName() : TEXT("None"),
					*AllNames,
					bHasFlash ? TEXT("예") : TEXT("아니오"),
					bHasDissolve ? TEXT("예") : TEXT("아니오"),
					(int32)DynMat->GetBlendMode());
			}
		}
	}
}

void AMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 서브클래스 BeginPlay(무기박스 SetCollisionProfileName 등)까지 전부 끝난 뒤,
	// 첫 Tick에서 한 번만 몬스터의 모든 콜리전 컴포넌트가 스프링암 카메라 콜리전 테스트(ECC_Camera)를
	// 무시하도록 강제한다. BeginPlay에서 하면 서브클래스가 나중에 SetCollisionProfileName으로
	// 이 설정을 덮어써버려서(예: WeaponBox를 "MonsterAttack" 프로필로 재설정) 공격 중 무기박스가
	// 다시 카메라를 막는 문제가 있었다.
	if (!bAppliedCameraCollisionIgnore)
	{
		bAppliedCameraCollisionIgnore = true;

		TArray<UPrimitiveComponent*> PrimitiveComponents;
		GetComponents<UPrimitiveComponent>(PrimitiveComponents);
		for (UPrimitiveComponent* Prim : PrimitiveComponents)
		{
			if (Prim)
				Prim->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		}
	}

	UpdateHitFlash(DeltaTime);
	UpdateHitRecoil(DeltaTime);
	UpdateDeathSequence(DeltaTime);
	UpdateHpBarVisibility();
	UpdateFootsteps();
}

void AMonster::PlayAlertSFX()
{
	// 감지가 풀렸다 다시 잡힐 때마다 울면 시끄러우니 몇 초에 한 번만 운다
	const double Now = FPlatformTime::Seconds();

	if (SFXProfile.Alert == ESFX::None || Now - LastAlertTime < 4.0)
		return;

	LastAlertTime = Now;
	CapstoneSFX::PlayAt(this, SFXProfile.Alert, GetActorLocation(), 1.f, SFXProfile.VoicePitch);
}

void AMonster::PlayAttackSFX()
{
	if (SFXProfile.Attack == ESFX::None)
		return;

	CapstoneSFX::PlayAt(this, SFXProfile.Attack, GetActorLocation(), 1.f, SFXProfile.VoicePitch);
}

void AMonster::UpdateFootsteps()
{
	if (SFXProfile.Step == ESFX::None || bDissolving)
		return;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	const FVector Location = GetActorLocation();

	// 공중(넉백으로 날아감)이거나 서 있을 때는 걸음 거리를 쌓지 않는다
	if (!Movement || !Movement->IsMovingOnGround() || Movement->Velocity.SizeSquared2D() < FMath::Square(40.f))
	{
		LastFootstepLocation = Location;
		return;
	}

	FootstepDistance += FVector::Dist2D(Location, LastFootstepLocation);
	LastFootstepLocation = Location;

	if (FootstepDistance < SFXProfile.StepStride)
		return;

	FootstepDistance = 0.f;

	// 플레이어가 멀리 있으면 들리지도 않으니 오디오 소스를 만들지 않는다
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player || FVector::DistSquared(Player->GetActorLocation(), Location) > FMath::Square(3500.f))
		return;

	const float HalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;
	CapstoneSFX::PlayAt(this, SFXProfile.Step, Location - FVector(0.f, 0.f, HalfHeight), 1.f, 1.f);
}

FVector AMonster::GetLockOnPoint() const
{
	if (LockOnSocketName != NAME_None && GetMesh() && GetMesh()->DoesSocketExist(LockOnSocketName))
		return GetMesh()->GetSocketLocation(LockOnSocketName);

	return GetActorLocation() + FVector(0.f, 0.f, LockOnHeightOffset);
}

void AMonster::ApplyHitRecoil(const FVector& HitDirection, float Distance)
{
	if (!GetMesh() || Distance <= 0.f)
		return;

	const FVector Direction = HitDirection.GetSafeNormal2D();

	if (Direction.IsNearlyZero())
		return;

	// 메시는 캡슐(루트)의 자식이라 상대 위치 기준으로 옮겨야 하므로 월드 방향을 액터 로컬로 바꿔 쌓는다.
	// 죽는 순간의 결정타에도 반동이 들어가도록 사망(디졸브) 여부는 검사하지 않는다
	const FVector LocalDirection = GetActorTransform().InverseTransformVectorNoScale(Direction);

	HitRecoilOffset = (HitRecoilOffset + LocalDirection * Distance * HitRecoilScale).GetClampedToMaxSize(MaxHitRecoil);
}

void AMonster::UpdateHitRecoil(float DeltaTime)
{
	USkeletalMeshComponent* MonsterMesh = GetMesh();

	if (!MonsterMesh)
		return;

	if (HitRecoilOffset.IsNearlyZero(0.05f))
	{
		// 다 돌아왔으면 기준 위치로 정확히 되돌려 두고, 이후로는 메시를 건드리지 않는다
		if (bRecoilApplied)
		{
			HitRecoilOffset = FVector::ZeroVector;
			MonsterMesh->SetRelativeLocation(MeshBaseRelativeLocation);
			bRecoilApplied = false;
		}

		return;
	}

	// 맞은 프레임에는 밀린 자리를 그대로 보여주고(최대 변위), 그 뒤부터 감쇠시킨다
	MonsterMesh->SetRelativeLocation(MeshBaseRelativeLocation + HitRecoilOffset);
	bRecoilApplied = true;

	HitRecoilOffset = FMath::VInterpTo(HitRecoilOffset, FVector::ZeroVector, DeltaTime, HitRecoilReturnSpeed);
}

void AMonster::UpdateHpBarVisibility()
{
	if (!HpBarComponent)
		return;

	// UBTService_Detect가 fTraceRange 안에서 플레이어를 찾으면 SetTarget으로 채워두므로,
	// 이 값의 유무로 탐지 여부를 그대로 판단한다
	AMonsterAIController* pAIController = Cast<AMonsterAIController>(GetController());
	// 락온된 몬스터는 아직 플레이어를 감지하지 못했더라도 HP를 확인할 수 있어야 한다
	const bool bDetected = bLockedOn || (pAIController && pAIController->GetTarget() != nullptr);

	if (HpBarComponent->IsVisible() != bDetected)
		HpBarComponent->SetVisibility(bDetected);
}

void AMonster::ShowDamageNumber(float Damage)
{
	const int32 DamageValue = FMath::RoundToInt(Damage);

	if (DamageValue <= 0)
		return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);

	if (!IsValid(PC))
		return;

	// 몬스터 머리 위에서 시작. 넉백으로 몬스터가 밀려나도 숫자는 맞은 자리에서 뜬다
	const FVector SpawnLocation = GetActorLocation() + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + DamageNumberHeightOffset);

	UUI_DamageNumber::Spawn(PC, DamageNumberWidgetClass, DamageValue, SpawnLocation);
}

void AMonster::UpdateHitFlash(float DeltaTime)
{
	if (HitFlashTimeRemaining <= 0.f)
		return;

	HitFlashTimeRemaining = FMath::Max(0.f, HitFlashTimeRemaining - DeltaTime);

	const float FlashAmount = HitFlashDuration > 0.f ? HitFlashTimeRemaining / HitFlashDuration : 0.f;

	for (UMaterialInstanceDynamic* DynMat : HitFlashMaterials)
	{
		if (IsValid(DynMat))
			DynMat->SetScalarParameterValue(HitFlashParamName, FlashAmount);
	}
}

void AMonster::UpdateDeathSequence(float DeltaTime)
{
	if (!bDissolving)
		return;

	DissolveTimeElapsed += DeltaTime;

	const float DissolveAmount = DissolveDuration > 0.f ? FMath::Clamp(DissolveTimeElapsed / DissolveDuration, 0.f, 1.f) : 1.f;

	for (UMaterialInstanceDynamic* DynMat : HitFlashMaterials)
	{
		if (IsValid(DynMat))
			DynMat->SetScalarParameterValue(DissolveParamName, DissolveAmount);
	}

	// [TEMP DEBUG] 스팸을 줄이기 위해 대략 0.5초 간격으로만 진행률 출력
	DebugDissolveLogAccum += DeltaTime;
	if (DebugDissolveLogAccum >= 0.5f)
	{
		DebugDissolveLogAccum = 0.f;
		LOG(TEXT("디졸브 진행중 (%s) DissolveAmount=%.2f"), *GetName(), DissolveAmount);
	}

	if (DissolveAmount >= 1.f)
	{
		LOG(TEXT("디졸브 완료, Destroy 호출 (%s)"), *GetName());
		Destroy();
	}
}

void AMonster::DropLoot()
{
    bool bDropped = false;

    for (const FMonsterDrop& Drop : DropTable)
    {
        if (Drop.ItemID.IsNone() || FMath::FRand() >= Drop.Chance)
            continue;

        const int32 MinCount = FMath::Max(1, Drop.MinCount);
        const int32 Count = FMath::RandRange(MinCount, FMath::Max(MinCount, Drop.MaxCount));

        // 여러 종류가 한꺼번에 떨어져도 한 점에 겹치지 않도록 둘레에 흩뿌린다
        const float Angle = FMath::FRandRange(0.f, 2.f * PI);
        const FVector Scatter(FMath::Cos(Angle) * 70.f, FMath::Sin(Angle) * 70.f, 0.f);

        if (AItemBase::SpawnLoot(this, Drop.ItemID, Count, Scatter))
            bDropped = true;
    }

    if (bDropped)
        CapstoneSFX::PlayAt(this, ESFX::UI_ItemDrop, GetActorLocation());
}

float AMonster::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    // 이미 사망(디졸브 시작)한 몬스터는 추가 데미지를 무시한다. 무기 콜리전 박스가 몬스터의
    // 캡슐/메시 등 여러 콜리전 컴포넌트와 동시에 겹치면 한 스윙에 WepaonBeginOverlap이 두 번
    // 호출될 수 있는데, 이때 사망 처리(OnMonsterKilled 방송 포함)가 중복 실행되어 퀘스트
    // 킬 카운트가 한 마리당 2씩 올라가는 버그의 원인이었다
    if (bDissolving)
        return 0.f;

    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    // 여기서 다시 m_tInfo.iHp를 빼면 안 된다 - Super::TakeDamage(ACharBase)가 이미 실제 피해량만큼
    // 차감을 끝냈다. 예전엔 여기서 또 빼서 피해가 두 배로 들어가는 버그가 있었다
    // (HP가 10 남았다고 로그가 찍히자마자 바로 디졸브가 발동하는 원인이었음)

    if (IsValid(HpBarWidget))
    {
        HpBarWidget->SetHp(m_tInfo.iHp / static_cast<float>(m_tInfo.iMaxHp));
    }

    // 사망 여부와 상관없이(치명타도 숫자가 보여야 하므로) 실제로 들어간 피해량을 띄운다
    ShowDamageNumber(ActualDamage);

    if (m_tInfo.iHp <= 0)
    {
        m_tInfo.iHp = 0;

        OnMonsterKilled.Broadcast(MonsterID, ExpReward, GoldReward);

        if (bIsBoss)
        {
            OnBossDefeated.Broadcast(MonsterID);
        }

            if (DropItemClasses.Num() > 0 && FMath::FRand() <= DropChance)
            {
                int32 RandomIndex = FMath::RandRange(0, DropItemClasses.Num() - 1);
                TSubclassOf<AItemBase> ItemToDrop = DropItemClasses[RandomIndex];

                if (ItemToDrop)
                {
                    FVector SpawnLocation = GetActorLocation() + FVector({ 0.f,-10.f , 0.f });

                    FRotator SpawnRotation = FRotator::ZeroRotator;

                    GetWorld()->SpawnActor<AItemBase>(ItemToDrop, SpawnLocation, SpawnRotation);

                    CapstoneSFX::PlayAt(this, ESFX::UI_ItemDrop, SpawnLocation);

                    UE_LOG(LogTemp, Warning, TEXT("고블린이 아이템을 드랍했습니다!"));
                }
            }

        DropLoot();

        // 여기서 바로 Destroy하지 않는다. Super::TakeDamage(ACharBase)에서 이미 데스 애니메이션 상태로
        // 전환해 두었으니, 데스 모션과 동시에 디졸브를 시작하고 DissolveDuration이 지나면
        // 실제로 Destroy한다(UpdateDeathSequence)
        bDissolving = true;
        DissolveTimeElapsed = 0.f;

        CapstoneSFX::PlayAt(this, SFXProfile.Death, GetActorLocation(), 1.f, SFXProfile.VoicePitch);

        if (SFXProfile.bDissolveSound)
            CapstoneSFX::PlayAt(this, ESFX::Mon_Dissolve, GetActorLocation());

        // [TEMP DEBUG]
        LOG(TEXT("사망 & 디졸브 동시 시작 (%s) / 대상 다이나믹 머티리얼 개수=%d / DissolveDuration=%.2f"),
			*GetName(), HitFlashMaterials.Num(), DissolveDuration);
    }
    else
    {
        // 즉사가 아닐 때만 피격 리액션(Hit 몽타주/플래시)을 재생한다. 치명타에서도 이걸 그대로 부르면
        // Super::TakeDamage가 방금 설정한 데스 애니메이션 상태를 Hit 몽타주가 덮어써버려서
        // 죽는 모션이 재생될 기회 자체가 없었다
        Hit();

        CapstoneSFX::PlayAt(this, SFXProfile.Hit, GetActorLocation(), 1.f, SFXProfile.VoicePitch);
    }

   
    return ActualDamage;
}

void AMonster::AttackEnd()
{
	OnAttackEnd.Broadcast();

	for (FDelegateHandle& handle : AttackEndHandle)
	{
		OnAttackEnd.Remove(handle);
	}
}

void AMonster::Hit()
{
	UMonsterAnim* pMonAnim = Cast<UMonsterAnim >(GetMesh()->GetAnimInstance());

	bHit = true;
	HitFlashTimeRemaining = HitFlashDuration;

	// bHit은 UMonsterAnim이 Hit 몽타주의 실제 종료 시점(OnHitMontageEnded)에 맞춰 false로 되돌린다
	if (IsValid(pMonAnim))
		pMonAnim->Hit();
}
