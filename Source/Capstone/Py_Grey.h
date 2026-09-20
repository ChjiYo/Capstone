#pragma once

#include "CharBase.h"
#include "CombatInterface.h"
#include "Py_Grey.generated.h"

USTRUCT(BlueprintType)
struct FSkillSlotData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	UAnimMontage* SkillMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	float Cooldown = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	TSoftObjectPtr<UTexture2D> Icon;

	// 월드 시간(GetTimeSeconds) 기준. 에디터에서 건드릴 값이 아니라 런타임에 TryUseSkill이 갱신
	float LastUsedTime = -FLT_MAX;
};

UCLASS()
class CAPSTONE_API APy_Grey : public ACharBase, public ICombatInterface
{
	GENERATED_BODY()

public:
	APy_Grey();


protected:
	UPROPERTY(Category = Camera, VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* pCamera;

	UPROPERTY(Category = Arm, VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* pArm;

	UPROPERTY(Category = Collision, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* WeaponBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUI_HpBar> PlayerHpBarClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUI_Inventory> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUI_QuestTracker> QuestTrackerWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUI_ExpBar> ExpBarWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|FX")
	TSubclassOf<class AEffectDestroy> HitEffectClass;

	// 폭발 버프 여부와 무관하게 몬스터를 명중시킬 때마다 매번 스폰되는 기본 타격 이펙트(스파크+타격음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel")
	TSubclassOf<class AEffectDestroy> NormalHitEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUI_SkillBar> SkillBarWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUI_Minimap> MinimapWidgetClass;

	// 퀘스트 목표 지점 위에 "몇 m 남았는지"를 띄우는 월드 마커 위젯 (WBP_QuestWaypoint). 비워두면 마커를 만들지 않는다
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUI_QuestWaypoint> QuestWaypointWidgetClass;

	// 8단계 엔딩 화면. 보스 처치 후 EndingDelay초 뒤에 생성
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUI_Ending> EndingWidgetClass;

	// 엔딩 화면의 [메인으로] 버튼이 다시 로드할 레벨 이름
	UPROPERTY(EditAnywhere, Category = "UI")
	FName StartLevelName;

	// 사망 시 띄울 "OOO에 의해 사망하셨습니다." 위젯. 기본값은 위젯 블루프린트 없이 C++로 만든 기본 스타일이고,
	// 꾸미고 싶으면 UUI_Death를 부모로 한 WBP를 만들어 이 폰의 블루프린트에서 지정하면 됨
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Death")
	TSubclassOf<class UUI_Death> DeathWidgetClass;

	// 사망 후 리스폰하는 레벨. 게임을 처음 시작할 때와 같은 위치(태그 없는 PlayerStart)에서 되살아난다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	FName RespawnLevelName = TEXT("Lv_Village");

	// 사망 위젯을 만들 수 없을 때(DeathWidgetClass 미지정 등)만 쓰이는 자동 리스폰까지의 시간(초).
	// 위젯이 뜨는 평소에는 [계속] 버튼을 눌러야 리스폰한다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	float RespawnDelay = 4.f;

	// 사망 시 재생할 데스 애니메이션. 애님BP(AP_PyGrey)에 Death 상태가 없어도 재생되도록, 죽는 순간 메시를
	// 이 시퀀스의 단일 노드 재생으로 전환한다. 기본값은 생성자에서 Greystone 데스 시퀀스로 채워지며 BP에서 교체 가능
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Death")
	TObjectPtr<class UAnimSequence> DeathAnimation;

	UPROPERTY()
	class UUI_Death* DeathWidget = nullptr;

	// 보스 처치 연출을 감상할 시간(초) 후 엔딩 화면을 띄운다
	UPROPERTY(EditAnywhere, Category = "UI")
	float EndingDelay = 3.f;

	UPROPERTY()
	class UUI_HpBar* PlayerHpBar;

	UPROPERTY()
	class UUI_SkillBar* SkillBarWidget = nullptr;

	UPROPERTY()
	class UUI_Minimap* MinimapWidget = nullptr;

	UPROPERTY()
	class AQuestWaypoint* QuestWaypoint = nullptr;

	UPROPERTY()
	class AItemBase* CurrItem;

	UPROPERTY()
	class ANPC_Base* CurrNPC;

	UPROPERTY()
	UUI_Inventory* InventoryWidget = nullptr;

	UPROPERTY()
	class UUI_QuestTracker* QuestTrackerWidget = nullptr;

	UPROPERTY()
	class UUI_ExpBar* ExpBarWidget = nullptr;

	UPROPERTY()
	class UUI_Ending* EndingWidget = nullptr;


private:
	UPROPERTY(EditDefaultsOnly, Category = "Dash|Animation")
	UAnimMontage* DashForwardMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Dash|Animation")
	UAnimMontage* DashBackwardMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Dash|Animation")
	UAnimMontage* DashLeftMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Dash|Animation")
	UAnimMontage* DashRightMontage;


private:
	bool bIsExplosiveBuffActive = false;
	bool bGameplayInputLocked = false;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UInventoryComponent* InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UQuestComponent* QuestComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UExperienceComponent* ExperienceComponent;

	// 11단계 대사/연출 재생기
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStoryComponent* StoryComponent;

	// 보스 유언 대사 (DL_BOSS_DEAD_01). 전투 연출 위에 겹쳐 나와야 하므로 입력을 막지 않는다
	UPROPERTY(EditAnywhere, Category = "Story")
	TArray<FName> BossDeathDialogueLines;

	// 엔딩 화면 직전 나레이션 (DL_EPI_01~03)
	UPROPERTY(EditAnywhere, Category = "Story")
	TArray<FName> EpilogueDialogueLines;

	// 레벨업 1회당 증가하는 체력/공격력. 체력은 최대치와 현재치에 동일하게 더해진다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	int32 HpIncreasePerLevel = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	int32 AttackIncreasePerLevel = 5;

	// 강화 성공 시 전역 방송 (NewLevel). 5단계 강화 퀘스트가 구독하는 static 멀티캐스트 델리게이트
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnWeaponEnhanced, int32 /*NewLevel*/);
	static FOnWeaponEnhanced OnWeaponEnhanced;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enhance")
	int32 WeaponLevel = 0;

	UPROPERTY(EditAnywhere, Category = "Enhance")
	FName EnhanceMaterialID = "Loot_GoblinEar";

	UPROPERTY(EditAnywhere, Category = "Enhance")
	int32 EnhanceMaterialCount = 3;

	UPROPERTY(EditAnywhere, Category = "Enhance")
	int32 EnhanceGoldCost = 50;

	UPROPERTY(EditAnywhere, Category = "Enhance")
	int32 AttackPerEnhance = 10;

	UPROPERTY(EditAnywhere, Category = "Enhance")
	int32 MaxWeaponLevel = 5;

	// 다음 강화에 필요한 골드 비용 (레벨이 오를수록 배율 증가)
	UFUNCTION(BlueprintPure, Category = "Enhance")
	int32 GetNextEnhanceGoldCost() const;

	UFUNCTION(BlueprintPure, Category = "Enhance")
	bool CanEnhanceWeapon() const;

	// 강화 시도. 재료/골드 검사 -> 소모 -> 공격력 증가 -> 방송. 성공 여부 반환
	UFUNCTION(BlueprintCallable, Category = "Enhance")
	bool TryEnhanceWeapon();

	// A/B/C 슬롯별 몽타주/쿨다운/아이콘. TryUseSkill이 쿨다운을 검사·갱신
	// 정적 배열은 블루프린트에 노출할 수 없어 EditAnywhere만 사용
	UPROPERTY(EditAnywhere, Category = "Skill")
	FSkillSlotData SkillSlots[3];

	// 대시로 회피 시작 시점에 전역 방송 (7단계 적응형 AI가 구독 예정). AMonster::OnMonsterKilled 패턴
	DECLARE_MULTICAST_DELEGATE(FOnPlayerDodge);
	static FOnPlayerDodge OnPlayerDodge;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
	float InvincibleDuration = 0.4f;

	// 공중(점프/낙하 중) 대시 발사 속도에 곱해지는 배율. 감속은 지상과 동일한 값을 빌려쓰므로,
	// 이 배율만 올리면 착지 거리를 자연스럽게 지상 대시보다 더 멀리 늘릴 수 있다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
	float AirDashSpeedMultiplier = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel")
	float HitStopTimeDilation = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel")
	float HitStopDuration = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel")
	TSubclassOf<class UCameraShakeBase> WeakCameraShakeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel")
	TSubclassOf<class UCameraShakeBase> StrongCameraShakeClass;

	// 콤보 한 타당 카메라 셰이크/히트스탑이 강해지는 배율. 콤보가 이어질수록 타격감이 점점 커지는 연출용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel")
	float ComboFeelScalePerHit = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel")
	float MaxComboFeelScale = 2.f;

	// 타격/피격 시 컨트롤러 진동 세기(0~1) 및 지속시간(초). 에셋 없이 코드만으로 동작
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel")
	float HitLandedRumbleIntensity = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel")
	float HitTakenRumbleIntensity = 0.6f;

	class UPy_GreyAnim* pGreyAnim;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void SetInteractableItem(class AItemBase* Item) { CurrItem = Item; }
	void SetInteractableNPC(class ANPC_Base* NPC) { CurrNPC = NPC; }

	
	virtual void SetExplosiveBuffState(bool bIsActive) override;

	// 스킬 B(보호막) 이펙트가 InitializeEffect에서 호출. 이미 보호막이 있으면 새 것으로 교체된다
	virtual void ActivateShield(int32 Amount, float Duration, class AEffectDestroy* ShieldEffect) override;

	UFUNCTION(BlueprintPure, Category = "Skill|Shield")
	bool HasShield() const { return CurrentShield > 0.f; }

	UFUNCTION(BlueprintPure, Category = "Skill|Shield")
	float GetCurrentShield() const { return CurrentShield; }

	// 인벤토리/상점 등 UI가 열려 있는 동안 공격·스킬 입력만 막는다 (이동은 계속 허용)
	void SetGameplayInputLocked(bool bLocked) { bGameplayInputLocked = bLocked; }

	// OpenLevel(하드 레벨 전환)은 폰과 컴포넌트를 전부 파괴하므로, 인벤토리/경험치/체력/공격력/무기 강화 단계를 게임 인스턴스
	// 서브시스템에 맡겨 새 폰이 BeginPlay에서 복원하게 한다. 포탈 이동과 사망 리스폰이 함께 쓴다
	// (퀘스트는 UQuestComponent가 상태가 바뀔 때마다 UQuestStateSubsystem에 직접 저장하므로 여기서 다루지 않는다)
	void SaveStateForLevelTransition(class ULevelTransitionSubsystem& TransitionSubsystem) const;

	// 사망 후 폰이 살아있어야 상태를 저장하고 리스폰할 수 있으므로, 데스 노티파이가 폰을 Destroy하지 않게 한다
	virtual bool ShouldDestroyOnDeathNotify() const override { return false; }

	UFUNCTION(BlueprintPure, Category = "Death")
	bool IsDead() const { return bIsDead; }


	// 특정 포션 아이템을 사용(회복 + 소모). 인벤토리 슬롯 클릭 등 UI에서도 호출할 수 있도록 public
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UsePotionItem(FName ItemID);

	// SlotIndex(0=A/1=B/2=C) 쿨다운 검사 후 LastUsedTime 갱신. 쿨다운 중이면 false
	UFUNCTION(BlueprintCallable, Category = "Skill")
	bool TryUseSkill(int32 SlotIndex);

	// UI가 Tick에서 폴링해서 쿨다운 오버레이를 갱신하는 용도
	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetSkillCooldownRemaining(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Skill")
	float GetSkillCooldown(int32 SlotIndex) const;

	const FSkillSlotData* GetSkillSlots() const { return SkillSlots; }

	// 스킬 A(공격력 강화) 사용 시 지속시간 동안 공격력에 곱해지는 배율 (1.5 = 50% 증가)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|AttackBuff")
	float AttackBuffMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|AttackBuff")
	float AttackBuffDuration = 10.f;

	UFUNCTION(BlueprintPure, Category = "Skill|AttackBuff")
	bool IsAttackBuffActive() const { return bAttackBuffActive; }

	// 공격력 강화 버프가 반영된, 실제 타격에 쓰이는 공격력
	UFUNCTION(BlueprintPure, Category = "Skill|AttackBuff")
	float GetEffectiveAttackPower() const;



private:
	UFUNCTION()
	void RefreshQuestTracker();

	UFUNCTION()
	void RefreshExpBar();

	UFUNCTION()
	void HandleLevelUp(int32 NewLevel);

	void HandleBossDefeated(FName MonsterID);
	void PlayEpilogue();
	void ShowEnding();

	// 체력이 0이 되면 한 번만 호출: 입력/이동을 막고 사망 문구+[계속] 버튼을 띄운다. 버튼을 누르면 RespawnAtVillage 실행
	void HandleDeath(AController* EventInstigator, AActor* DamageCauser);
	void RespawnAtVillage();

	bool bIsDead = false;
	FTimerHandle RespawnTimerHandle;

	FDelegateHandle BossDefeatedHandle;
	FTimerHandle EndingTimerHandle;

	// 재사용하면 지속시간만 갱신되고 배율은 중첩되지 않는다
	void ActivateAttackBuff();
	void EndAttackBuff();

	bool bAttackBuffActive = false;
	FTimerHandle AttackBuffTimerHandle;

	// 보호막이 깨지거나 지속시간이 끝나면 호출: 수치 초기화 + 보호막 VFX 액터 파괴 + 게이지 숨김
	void EndShield();
	void RefreshShieldUI();

	float CurrentShield = 0.f;
	float MaxShield = 0.f;
	FTimerHandle ShieldTimerHandle;
	TWeakObjectPtr<class AEffectDestroy> ShieldEffectActor;

private:
	void MoveFront(float _fScale);
	void MoveSide(float _fScale);
	void RotationZ(float _fScale);
	void RotationX(float _fScale);

private:
	void JumpKey();
	void NormalAttack();
	void SkillSlotA();
	void SkillSlotB();
	void SkillSlotC();
	void OnInteractKeyPressed();
	void ToggleInventory();
	void UsePotion();

	void RollOrDash();

private:
	bool bInvincible = false;
	FTimerHandle InvincibilityTimerHandle;
	FTimerHandle HitStopTimerHandle;
	FTimerHandle AirDashBrakingTimerHandle;
	float DefaultBrakingDecelerationFalling = 0.f;
	float DefaultFallingLateralFriction = 0.f;

	void EndInvincibility();
	void RestoreAirBraking();
	void PlayHitStop(float DurationScale = 1.f);
	void EndHitStop();
	void PlayCameraShake(TSubclassOf<class UCameraShakeBase> ShakeClass, float Scale = 1.f);
	void PlayHitRumble(float Intensity, float Duration);

public:
	void EnableWeaponCollision(bool bEnable);

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

protected:
	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
};
