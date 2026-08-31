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

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUI_SkillBar> SkillBarWidgetClass;

	// 8단계 엔딩 화면. 보스 처치 후 EndingDelay초 뒤에 생성
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUI_Ending> EndingWidgetClass;

	// 엔딩 화면의 [메인으로] 버튼이 다시 로드할 레벨 이름
	UPROPERTY(EditAnywhere, Category = "UI")
	FName StartLevelName;

	// 보스 처치 연출을 감상할 시간(초) 후 엔딩 화면을 띄운다
	UPROPERTY(EditAnywhere, Category = "UI")
	float EndingDelay = 3.f;

	UPROPERTY()
	class UUI_HpBar* PlayerHpBar;

	UPROPERTY()
	class UUI_SkillBar* SkillBarWidget = nullptr;

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
	int32 HpIncreasePerLevel = 50;

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

	// 인벤토리/상점 등 UI가 열려 있는 동안 공격·스킬 입력만 막는다 (이동은 계속 허용)
	void SetGameplayInputLocked(bool bLocked) { bGameplayInputLocked = bLocked; }

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

	FDelegateHandle BossDefeatedHandle;
	FTimerHandle EndingTimerHandle;

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
	void PlayHitStop();
	void EndHitStop();
	void PlayCameraShake(TSubclassOf<class UCameraShakeBase> ShakeClass);

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
