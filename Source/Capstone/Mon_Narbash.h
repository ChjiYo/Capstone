#pragma once

#include "Monster.h"
#include "Mon_Narbash.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBossSkillFinished, bool /*bCompleted*/)

// 나르바쉬 보스가 쓰는 스킬. Combo~Rampage는 UCombatPatternSubsystem의 패턴 인덱스(= enum 값)로도 쓰여서
// 플레이어가 잘 맞아주는/못 맞는 스킬을 학습해 선택 가중치에 반영한다 (7단계 적응형 AI)
UENUM(BlueprintType)
enum class ENarbashSkill : uint8
{
	None,
	Roar,		// 조우 시 포효 / 2페이즈 전환 포효 (충격파로 플레이어를 밀어냄)
	Combo,		// 근접 3연타 (기존 몽타주, 전진 러시 추가)
	Smash,		// 내려찍기: 바닥 예고 후 광역 충격 (2페이즈에서는 이어서 충격파 링)
	Charge,		// 돌진: 직선 경로 예고 후 돌진, 명중 시 큰 피해+넉백 (2페이즈에서는 연속 돌진)
	LeapSlam,	// 도약 후 착지 광역 (착지 지점 예고)
	DrumThrow,	// 북채 투척 (원거리 견제. 2페이즈에서는 3갈래)
	Rampage,	// 2페이즈 전용 궁극기: 연속 지면 강타 + 퍼지는 충격파(점프/회피로 피할 수 있음)
	Count UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct FNarbashSkillConfig
{
	GENERATED_BODY()

	// 이 거리(수평, cm) 안에 있을 때 바로 시전할 수 있다. 벗어나 있으면 접근하거나 다른 스킬을 고른다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float MinRange = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float MaxRange = 300.f;

	// 시전 후 다시 쓸 수 있을 때까지의 시간(초). 2페이즈에서는 Phase2CooldownScale이 곱해진다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float Cooldown = 0.f;

	// 스킬 선택 기본 가중치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	int32 MinPhase = 1;

	// 기본 공격력(m_tInfo.iAttack)에 곱해지는 피해 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float DamageMultiplier = 1.f;
};

UCLASS()
class CAPSTONE_API AMon_Narbash : public AMonster
{
	GENERATED_BODY()

public:
	AMon_Narbash();

protected:
	class UMon_NarbashAnim* pNarbashAnim;

	UPROPERTY(Category = Collision, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* LeftWeaponBox;

	UPROPERTY(Category = Collision, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* RightWeaponBox;

	UPROPERTY(Category = Collision, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USphereComponent* DashSphere;

	// LaunchCharacter로 돌진 중인 동안 true. (구 돌진 몽타주용) Tick에서 근접 사거리 도달/타겟 상실 여부를 체크해 돌진을 멈추는 데 쓰인다
	bool bIsDashing = false;

	// 이번 스윙에서 이미 피해를 준 액터. 좌/우 무기 박스와 대상의 캡슐/메시 등 여러 콜리전 컴포넌트가
	// 겹치면 한 스윙에 오버랩 이벤트가 여러 번 오기 때문에, 대상당 한 번만 맞도록 걸러내는 데 쓴다.
	// 무기 콜리전을 켤 때(콤보 섹션 시작마다)마다 초기화된다
	TSet<TWeakObjectPtr<AActor>> HitActorsThisSwing;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;

protected:
	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

public:
	void EnableWeaponCollision(bool bEnable);
	void ExecutePhysicalDash();

	// 보스는 무거워서 플레이어 스킬 등 외부의 넉백/띄우기에 밀리지 않는다 (스킬 내부의 도약/러시는 InternalLaunch로 허용)
	virtual void LaunchCharacter(FVector LaunchVelocity, bool bXYOverride, bool bZOverride) override;

	// 보스는 피격 시 동작이 끊기지 않는다(슈퍼아머). 피격 플래시만 재생하고, 누적 강인도(Poise)가 바닥나면 스턴한다
	virtual void Hit() override;

	// =====================================================================
	// 보스 브레인 / 스킬 시스템
	// =====================================================================
public:
	// true면 BP의 BehaviorTreeAsset(BT_Narbash) 대신 코드로 조립한 보스 전용 트리를 전투 서브트리로 쓴다.
	// 새 트리에 문제가 있을 때 false로 돌리면 기존 BT로 즉시 되돌아간다
	UPROPERTY(EditAnywhere, Category = "Boss|AI")
	bool bUseCodeBuiltTree = true;

	// 스킬별 설정. 인덱스는 ENarbashSkill 값이다 (생성자에서 기본값을 채운다)
	UPROPERTY(EditAnywhere, Category = "Boss|AI")
	TArray<FNarbashSkillConfig> SkillConfigs;

	// 보스 전투 중 플레이어를 놓치지 않도록 탐지 범위를 최소 이 값까지 늘린다 (BP에 더 큰 값이 있으면 그대로 둔다)
	UPROPERTY(EditAnywhere, Category = "Boss|AI")
	float MinBossTraceRange = 3000.f;

	// 조우 포효를 터뜨리는 플레이어와의 거리(cm). 이보다 멀면 이 거리까지 접근한 뒤 포효한다
	UPROPERTY(EditAnywhere, Category = "Boss|AI")
	float IntroRoarDistance = 1500.f;

	// 스킬 직후 후딜(초): 이 시간 동안은 이동/공격 없이 플레이어를 노려본다. 2페이즈는 배율이 곱해진다
	UPROPERTY(EditAnywhere, Category = "Boss|AI")
	FVector2D RecoveryTimeRange = FVector2D(0.7f, 1.4f);

	UPROPERTY(EditAnywhere, Category = "Boss|AI")
	float Phase2RecoveryScale = 0.45f;

	UPROPERTY(EditAnywhere, Category = "Boss|AI")
	float Phase2CooldownScale = 0.7f;

	// 후딜/대기 중 플레이어를 바라보는 회전 속도(도/초)
	UPROPERTY(EditAnywhere, Category = "Boss|AI")
	float RecoveryTurnRate = 200.f;

	// 강인도: 이 값만큼 피해를 누적해서 받으면 스턴. 스턴 중에는 받는 피해가 늘어나 플레이어에게 보상이 된다
	UPROPERTY(EditAnywhere, Category = "Boss|Poise")
	float MaxPoise = 240.f;

	UPROPERTY(EditAnywhere, Category = "Boss|Poise")
	float PoiseRegenPerSecond = 30.f;

	UPROPERTY(EditAnywhere, Category = "Boss|Poise")
	float PoiseRegenDelay = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Boss|Poise")
	float StaggerDuration = 2.4f;

	UPROPERTY(EditAnywhere, Category = "Boss|Poise")
	float StaggerDamageMultiplier = 1.3f;

	// 스킬 튜닝 값 (애니메이션 길이 대비 비율/시간/속도) ------------------------
	// 내려찍기: 애니메이션 전체 길이 중 실제로 바닥을 때리는 시점의 비율
	UPROPERTY(EditAnywhere, Category = "Boss|Skill Tuning")
	float SmashStrikeFraction = 0.45f;

	UPROPERTY(EditAnywhere, Category = "Boss|Skill Tuning")
	float SmashForwardOffset = 210.f;

	UPROPERTY(EditAnywhere, Category = "Boss|Skill Tuning")
	float SmashRadius = 320.f;

	// 콤보 몽타주 기본 재생 속도 (페이즈 배율이 추가로 곱해진다)
	UPROPERTY(EditAnywhere, Category = "Boss|Skill Tuning")
	float ComboBaseRate = 1.15f;

	// 돌진: 경로 예고 시간, 돌진 속도, 명중 판정 반경
	UPROPERTY(EditAnywhere, Category = "Boss|Skill Tuning")
	float ChargeWindupTime = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Boss|Skill Tuning")
	float ChargeSpeed = 2600.f;

	UPROPERTY(EditAnywhere, Category = "Boss|Skill Tuning")
	float ChargeHitRadius = 150.f;

	// 도약: 웅크리는 시간, 체공 시간, 착지 광역 반경
	UPROPERTY(EditAnywhere, Category = "Boss|Skill Tuning")
	float LeapWindupTime = 0.7f;

	UPROPERTY(EditAnywhere, Category = "Boss|Skill Tuning")
	float LeapAirTime = 0.8f;

	UPROPERTY(EditAnywhere, Category = "Boss|Skill Tuning")
	float LeapRadius = 380.f;

	// 투척: 애니메이션 중 북채를 놓는 시점의 비율, 북채 속도
	UPROPERTY(EditAnywhere, Category = "Boss|Skill Tuning")
	float ThrowReleaseFraction = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Boss|Skill Tuning")
	float ThrowSpeed = 1900.f;

	// BT 인터페이스 ------------------------------------------------------
	virtual class UBehaviorTree* GetCombatBehaviorTree(class UBlackboardData* BlackboardAsset) override;

	// 스킬 시전 중이거나 스턴 중이면 true (이동/새 스킬 불가)
	bool IsBusy() const { return ActiveSkill != ENarbashSkill::None || bStaggered; }
	bool IsStaggered() const { return bStaggered; }
	bool IsRecovering() const { return RecoveryRemaining > 0.f; }

	// 미리 골라 둔 다음 스킬을 지금 바로 쓸 수 있는가 (사거리/쿨다운/후딜/스턴)
	bool IsPendingSkillReady() const;

	// 다음 스킬을 쓰기 위해 접근해야 하는 거리(cm)
	float GetPendingApproachDistance() const;

	float GetDistanceToTarget2D() const;

	// 다음 스킬을 시전한다. 스킬이 끝나면 OnSkillFinished가 방송된다
	bool BeginPendingSkill();
	void CancelSkill();

	FOnBossSkillFinished OnSkillFinished;

	// 플레이어 쪽으로 회전 속도 제한을 두고 몸을 돌린다 (도/초)
	void FaceTarget(float DeltaTime, float TurnRate);

	// 애니메이션 노티파이가 부르는 콤보 훅 -----------------------------------
	void OnComboSwingBegin();
	void OnComboSwingWindowEnd();
	float GetComboChainChance() const;

	// 투사체가 플레이어에게 명중/빗나갔을 때 적응형 학습에 반영
	void NotifyProjectileResult(bool bDidHit);

	ENarbashSkill GetActiveSkill() const { return ActiveSkill; }
	int32 GetPhase() const { return CurrentPhase; }

	// =====================================================================
	// 페이즈
	// =====================================================================
public:
	// HP 비율이 이 값 이하로 떨어지면 2페이즈로 전환 (8단계 보스전)
	UPROPERTY(EditAnywhere, Category = "Boss")
	float Phase2HpRatio = 0.5f;

	// 2페이즈 전용 공격 패턴. 전환 시 AttackPatterns를 이걸로 교체 (7단계 학습은 계속 적용됨)
	UPROPERTY(EditAnywhere, Category = "Boss")
	TArray<UAnimMontage*> Phase2AttackPatterns;

	UPROPERTY(EditAnywhere, Category = "Boss")
	float Phase2SpeedMultiplier = 1.3f;

	// (구 방식) 전환 연출 몽타주. 새 브레인은 코드 애니메이션(포효 스킬)을 쓰므로 지정하지 않아도 된다
	UPROPERTY(EditAnywhere, Category = "Boss")
	UAnimMontage* PhaseTransitionMontage;

	UPROPERTY(EditAnywhere, Category = "Boss")
	TSubclassOf<class UCameraShakeBase> PhaseTransitionCameraShake;

	// 전환 연출 동안 무적 처리할 시간(초). 포효 스킬이 끝나면 그 시점에 무적이 풀리고, 이 값은 안전장치 상한이다
	UPROPERTY(EditAnywhere, Category = "Boss")
	float PhaseTransitionInvincibleDuration = 4.f;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UUI_BossHpBar> BossHpBarClass;

	// 11단계: 2페이즈 전환 시 재생되는 대사. 전투 중이라 입력을 막지 않고 AutoAdvanceTime으로 자동 진행된다
	UPROPERTY(EditAnywhere, Category = "Boss|Story")
	TArray<FName> Phase2DialogueLines;

	int32 CurrentPhase = 1;

private:
	bool bPhaseTransitioning = false;
	bool bPhaseRoarPending = false;
	FTimerHandle PhaseTransitionTimerHandle;

	UPROPERTY()
	class UUI_BossHpBar* BossHpBarWidget = nullptr;

	void EnterPhase2();
	void EndPhaseTransition();

	// =====================================================================
	// 내부 구현: 애셋 (파라곤 나르바쉬)
	// =====================================================================
private:
	UPROPERTY()
	UAnimSequence* Anim_Roar = nullptr;
	UPROPERTY()
	UAnimSequence* Anim_Smash = nullptr;
	UPROPERTY()
	UAnimSequence* Anim_ChargeIntro = nullptr;
	UPROPERTY()
	UAnimSequence* Anim_ChargeRun = nullptr;
	UPROPERTY()
	UAnimSequence* Anim_ChargeStop = nullptr;
	UPROPERTY()
	UAnimSequence* Anim_LeapStart = nullptr;
	UPROPERTY()
	UAnimSequence* Anim_LeapAir = nullptr;
	UPROPERTY()
	UAnimSequence* Anim_LeapLand = nullptr;
	UPROPERTY()
	UAnimSequence* Anim_Throw = nullptr;
	UPROPERTY()
	UAnimSequence* Anim_RampageIntro = nullptr;
	UPROPERTY()
	UAnimSequence* Anim_RampageLoop = nullptr;
	UPROPERTY()
	UAnimSequence* Anim_RampageEnd = nullptr;
	UPROPERTY()
	UAnimSequence* Anim_StunStart = nullptr;
	UPROPERTY()
	UAnimSequence* Anim_StunLoop = nullptr;
	UPROPERTY()
	UAnimSequence* Anim_StunEnd = nullptr;

	UPROPERTY()
	UParticleSystem* FX_MeleeImpact = nullptr;
	UPROPERTY()
	UParticleSystem* FX_MeleeTrail = nullptr;
	UPROPERTY()
	UParticleSystem* FX_SeismicImpact = nullptr;
	UPROPERTY()
	UParticleSystem* FX_BigKnockup = nullptr;
	UPROPERTY()
	UParticleSystem* FX_EnergyBuildup = nullptr;
	UPROPERTY()
	UParticleSystem* FX_DashCast = nullptr;
	UPROPERTY()
	UParticleSystem* FX_DashGround = nullptr;
	UPROPERTY()
	UParticleSystem* FX_UltLoop = nullptr;
	UPROPERTY()
	UParticleSystem* FX_UltEnd = nullptr;
	UPROPERTY()
	UParticleSystem* FX_MaceClacks = nullptr;
	UPROPERTY()
	UParticleSystem* FX_Aura = nullptr;
	UPROPERTY()
	UParticleSystem* FX_AuraEnergy = nullptr;

	UPROPERTY()
	USoundBase* SFX_Roar = nullptr;
	UPROPERTY()
	USoundBase* SFX_EffortHeavy = nullptr;
	UPROPERTY()
	USoundBase* SFX_EffortSwing = nullptr;
	UPROPERTY()
	USoundBase* SFX_EffortCharge = nullptr;
	UPROPERTY()
	USoundBase* SFX_Pain = nullptr;

	UPROPERTY()
	USoundBase* SFX_Impact = nullptr;

	// =====================================================================
	// 내부 구현: 브레인
	// =====================================================================
private:
	ENarbashSkill ActiveSkill = ENarbashSkill::None;
	ENarbashSkill PendingSkill = ENarbashSkill::None;
	ENarbashSkill LastSkill = ENarbashSkill::None;
	ENarbashSkill LastLastSkill = ENarbashSkill::None;

	float PendingAge = 0.f;
	float RecoveryRemaining = 0.f;
	TArray<float> SkillCooldowns;

	bool bEngaged = false;
	bool bDead = false;
	bool bArmored = false;
	bool bSkillConnected = false;
	bool bStaggered = false;
	float StaggerElapsed = 0.f;
	int32 StaggerStage = 0;

	float Poise = 0.f;
	float TimeSinceLastHit = 0.f;

	FName MontageSlotName = TEXT("DefaultSlot");

	// 스킬 타임라인: 스킬 시작 시점 기준 상대 시간(초)에 실행할 동작을 등록해 두고 Tick에서 순서대로 실행한다
	struct FSkillEvent
	{
		float Time = 0.f;
		TFunction<void()> Action;
		bool bFired = false;
	};

	TArray<FSkillEvent> SkillTimeline;
	float SkillElapsed = 0.f;

	// 스킬 진행 중에 만든 예고 표시/지속 이펙트. 스킬이 끊기면 함께 치운다
	TArray<TWeakObjectPtr<class ABossTelegraph>> ActiveTelegraphs;
	TArray<TWeakObjectPtr<UParticleSystemComponent>> ActiveSkillFX;

	// 스킬 시작마다 증가하는 번호. 타임라인 이벤트 실행 중에 스킬이 끝났다가 새로 시작된 경우를 구분한다
	int32 SkillSerial = 0;

	// 코드로 재생한 동적 몽타주 (재생 속도 조절/추적용)
	UPROPERTY()
	UAnimMontage* CurrentDynamicMontage = nullptr;
	float ActiveSkillMontageRateHint = 1.f;

	// 사망 슬로모션
	bool bTimeDilated = false;
	FTimerHandle DeathSlowmoTimerHandle;

	// 회전: 기본은 타겟 추적, bTrackFixedDir이면 지정한 방향으로 돈다 (돌진 경로 정렬용)
	bool bTracking = false;
	bool bTrackFixedDir = false;
	FVector TrackFixedDir = FVector::ForwardVector;
	float TrackTurnRate = 240.f;

	bool bChargeHitThisRun = false;

	// 스턴 연출
	float StaggerStartLen = 0.5f;
	float StaggerEndLen = 0.5f;

	// 돌진 상태
	bool bChargeRunning = false;
	bool bChargeSecondRun = false;
	FVector ChargeDir = FVector::ForwardVector;
	FVector ChargeLastLocation = FVector::ZeroVector;
	float ChargeTraveled = 0.f;
	float ChargeMaxDistance = 1500.f;
	float ChargeRunElapsed = 0.f;
	float ChargeBlockedTime = 0.f;
	float SavedMaxWalkSpeed = 0.f;
	float SavedMaxAcceleration = 0.f;
	bool bSavedOrientToMovement = false;
	bool bChargeMovementOverridden = false;

	// 도약 상태
	bool bLeapAirborne = false;
	float LeapAirElapsed = 0.f;
	FVector LeapGoal = FVector::ZeroVector;

	// 충격파 링 (Rampage/2페이즈 스매시): 퍼져나가는 링에 지상의 플레이어가 닿으면 피해
	struct FShockRing
	{
		FVector Center = FVector::ZeroVector;
		float Elapsed = 0.f;
		float Duration = 0.8f;
		float MaxRadius = 700.f;
		float Damage = 20.f;
		bool bHit = false;
	};

	TArray<FShockRing> ShockRings;

	// 조우 시 포효와 2페이즈 광폭화 오라
	UPROPERTY()
	UParticleSystemComponent* AuraComponent = nullptr;

	UPROPERTY()
	UParticleSystemComponent* AuraEnergyComponent = nullptr;

	// 기본 스킬 설정값 채우기
	void InitSkillConfigs();
	void LoadBossAssets();

	// 지정한 방향(평면)으로 회전 속도 제한을 두고 몸을 돌린다
	void FaceDirection(const FVector& Direction, float DeltaTime, float TurnRate);

	// 회피를 자주 하는 플레이어에게 공격 시작을 늦추는 시간(초). 0이면 지연 없음
	float RollFeintDelay() const;

	// 브레인
	void UpdateBrain(float DeltaTime);
	ENarbashSkill PickNextSkill();
	bool IsSkillOffCooldown(ENarbashSkill Skill) const;
	const FNarbashSkillConfig& GetConfig(ENarbashSkill Skill) const;
	ACharacter* GetTargetChar() const;
	float GetSkillSpeed() const { return CurrentPhase >= 2 ? 1.15f : 1.f; }

	// 스킬 실행
	bool BeginSkill(ENarbashSkill Skill);
	void EndSkill(bool bCompleted);
	void At(float Time, TFunction<void()> Action);
	void TickSkillTimeline(float DeltaTime);

	void StartRoar(bool bPhaseTransition);
	void StartCombo();
	void StartSmash();
	void StartCharge();
	void StartLeapSlam();
	void StartDrumThrow();
	void StartRampage();

	// 돌진/도약 세부
	void BeginChargeWindup(bool bSecond);
	void BeginChargeRun();
	void TickCharge(float DeltaTime);
	void EndChargeRun(bool bHitPlayer);
	void RestoreChargeMovement();
	void LaunchLeap();
	void TickLeap(float DeltaTime);
	void OnLeapLanded();

	// 스턴
	void EnterStagger();
	void TickStagger(float DeltaTime);
	void EndStagger();

	// 유틸
	float PlaySeq(UAnimSequence* Seq, float Rate = 1.f, float BlendIn = 0.12f, float BlendOut = 0.15f, int32 Loops = 1, float StartTime = 0.f);
	UParticleSystemComponent* SpawnFXAt(UParticleSystem* FX, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator, float Scale = 1.f);
	UParticleSystemComponent* SpawnFXAttached(UParticleSystem* FX, FName Socket, float Scale = 1.f, bool bTrackForCleanup = true);
	void PlaySFX(USoundBase* Sound, const FVector& Location, float Volume = 1.f);
	void ShakePlayerCamera(TSubclassOf<class UCameraShakeBase> ShakeClass, float Scale, float MaxDistance = 3000.f);
	class ABossTelegraph* TrackTelegraph(class ABossTelegraph* Telegraph);
	void ClearSkillVisuals();

	// 스킬 내부에서 쓰는 발사. 외부 넉백을 막는 LaunchCharacter 재정의를 통과시킨다
	void InternalLaunch(const FVector& Velocity, bool bXYOverride, bool bZOverride);
	bool bInternalLaunch = false;

	FVector GetGroundLocation(const FVector& Location) const;
	float GetGroundZUnderBoss() const;
	bool IsTargetInCircle(const FVector& Center, float Radius, float MaxHeightDiff = 260.f) const;
	float DamagePlayer(float Multiplier, const FVector& KnockbackDir, float KnockStrength, float KnockbackUp);
	void RecordSkillResult(bool bDidHit);
	void SpawnShockRing(const FVector& Center, float MaxRadius, float Duration, float DamageMultiplier);
	void TickShockRings(float DeltaTime);
	void PushPlayerAway(float Radius, float Strength, float Up);
	void ActivateEnrageAura();
	void OnBossDeath();

	// 코드로 조립한 전투 서브트리 (GC 방지를 위해 UPROPERTY로 보관)
	UPROPERTY()
	class UBehaviorTree* CodeBuiltTree = nullptr;

public:
	UFUNCTION()
	void  WepaonBeginOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent*
		OtherComp, int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void WeaponEndOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);


	UFUNCTION()
	void OnCollisionOverlap(UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor, UPrimitiveComponent* OtherComp, int32
		OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnComboMontageEnded(UAnimMontage* Montage, bool bInterrupted);
};
