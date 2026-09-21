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

	// 'U' 키로 여는 장비창. 기본값은 위젯 블루프린트 없이 C++로 조립하는 UUI_Equipment이고, 꾸미고 싶으면 UUI_Equipment를 부모로 한 WBP를 지정하면 된다
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUI_Equipment> EquipmentWidgetClass;

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
	class UUI_Equipment* EquipmentWidget = nullptr;

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

	// 착지 소리 크기를 정하기 위해 공중에 있는 동안 마지막 낙하 속도를 기억해 둔다 (Landed 시점엔 이미 0에 가깝다)
	float LastFallSpeedZ = 0.f;

	// 체력이 낮을 때 심장 박동 소리를 내는 실시간 타이머
	float HeartbeatTimer = 0.f;
	void UpdateLowHealthHeartbeat();

	// 스킬 쿨다운이 끝나는 순간 "준비됨" 소리를 내는 타이머 (A/B/C 슬롯별)
	FTimerHandle SkillReadyTimers[3];

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UInventoryComponent* InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UQuestComponent* QuestComponent;

	// 끼고 있는 검/방패와 그 보너스, 손에 보이는 모델
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UEquipmentComponent* EquipmentComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UExperienceComponent* ExperienceComponent;

	// 11단계 대사/연출 재생기
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStoryComponent* StoryComponent;

	// 락온(대상 고정 카메라) + 공격 어시스트 대상 탐색
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class ULockOnComponent* LockOnComponent;

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

	// 대시 이펙트. 기본값은 몽타주(Jog_*_Dash 시퀀스)의 AnimNotify_PlayNiagaraEffect가 재생하던 NS_Dash_Mana와 같은 시스템이다.
	// 그 노티파이는 앞/뒤/좌/우 4방향 중 가장 가까운 몽타주의 고정 방향으로만 재생돼서, 대각선 대시나 벽 근처 대시에서
	// 이펙트가 실제 이동 방향과 어긋나거나 벽 속에 박혔다. 이 값이 지정돼 있으면 노티파이 쪽 이펙트는 런타임에만 꺼두고
	// (에셋은 저장하지 않음) 여기서 실제 대시 방향에 맞춰 한 번 재생한다. 비워두면 예전처럼 몽타주 노티파이 이펙트를 그대로 쓴다.
	// 이펙트를 캐릭터에 붙여 끌고 다니면 Niagara가 컴포넌트의 이동 속도를 입자에 상속시켜 화면을 뒤덮는 줄무늬가 되므로 고정 스폰한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash|Effect")
	TObjectPtr<class UNiagaraSystem> DashEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash|Effect")
	FVector DashEffectScale = FVector(1.f, 1.f, 1.f);

	// 이펙트가 대시 진행 방향(X축)을 기준으로 놓이도록 더해주는 Yaw 보정. 이펙트 에셋이 다른 축을 정면으로 만들어져 있을 때 조절
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash|Effect")
	float DashEffectYawOffset = 0.f;

	// 발 밑 기준, 대시 방향으로 회전된 좌표계의 스폰 위치 (X=진행방향, Y=오른쪽, Z=위). 기본값은 출발점 뒤쪽 가슴 높이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash|Effect")
	FVector DashEffectOffset = FVector(-60.f, 0.f, 60.f);

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

	// ===== 타격 임팩트: 몬스터의 반응(넉백/메시 반동) + 카메라 펀치 + 처치 연출 =====
	// 이 번호(1부터)에 도달한 콤보 타를 마무리 타로 친다. 마무리 타와 스킬 C는 넉백/반동/카메라 펀치가 크게 들어간다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel|Impact")
	int32 FinisherComboIndex = 4;

	// 일반 타격이 몬스터의 KnockbackStrength에 곱해져 밀어내는 배율. 마무리 타는 FinisherKnockbackScale (보스는 밀리지 않음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel|Impact")
	float NormalKnockbackScale = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel|Impact")
	float FinisherKnockbackScale = 4.f;

	// 맞은 몬스터의 메시가 밀려나는 거리(cm). 콤보 배율이 곱해지고, 마무리 타는 FinisherRecoilScale배
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel|Impact")
	float HitRecoilDistance = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel|Impact")
	float FinisherRecoilScale = 2.f;

	// 명중 순간 카메라가 확 줌인(FOV 감소, 도)되고 타격 방향으로 밀려 들어가는 거리(cm). 이후 빠르게 제자리로 돌아온다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel|Impact")
	float CameraPunchFOV = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel|Impact")
	float CameraPunchKick = 8.f;

	// 카메라 펀치가 원래대로 돌아오는 속도 (1/초, 지수 감쇠). 실시간 기준이라 히트스톱 중에도 그대로 흐른다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel|Impact")
	float CameraPunchReturnSpeed = 14.f;

	// 몬스터를 처치한 타의 히트스톱 길이 배율(HitStopDuration 기준)과 카메라 셰이크 배율. 보스는 자체 사망 슬로모션이 있어 히트스톱을 더하지 않는다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel|Impact")
	float KillHitStopScale = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel|Impact")
	float KillCameraShakeScale = 1.5f;

	// ===== 공격 어시스트: 일반 공격 스윙이 시작될 때 대상 쪽으로 파고든다 =====
	// 대상은 락온 대상, 없으면 전방 AttackAssistHalfAngle 원뿔 안에서 AttackAssistRange(cm) 이내의 가장 가까운 몬스터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel|AttackAssist")
	float AttackAssistRange = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel|AttackAssist")
	float AttackAssistHalfAngle = 35.f;

	// 나와 대상의 캡슐 표면 사이가 이 거리(cm)가 될 때까지만 다가간다. 이미 이 안쪽이면 파고들지 않는다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel|AttackAssist")
	float AttackAssistStopGap = 70.f;

	// 한 스윙에 파고드는 최대 거리(cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel|AttackAssist")
	float AttackAssistMaxTravel = 220.f;

	// 일반 공격 스윙이 시작되는 순간 UPy_GreyAnim이 호출한다 (스킬 몽타주에서는 호출되지 않음)
	void ApplyAttackAssist();


	class UPy_GreyAnim* pGreyAnim;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 점프/착지 효과음 (ACharacter 훅)
	virtual void OnJumped_Implementation() override;
	virtual void Landed(const FHitResult& Hit) override;
	void SetInteractableItem(class AItemBase* Item) { CurrItem = Item; }
	void SetInteractableNPC(class ANPC_Base* NPC) { CurrNPC = NPC; }

	
	virtual void SetExplosiveBuffState(bool bIsActive) override;

	// 스킬 B(보호막) 이펙트가 InitializeEffect에서 호출. 이미 보호막이 있으면 새 것으로 교체된다
	virtual void ActivateShield(int32 Amount, float Duration, class AEffectDestroy* ShieldEffect) override;

	UFUNCTION(BlueprintPure, Category = "Skill|Shield")
	bool HasShield() const { return CurrentShield > 0.f; }

	UFUNCTION(BlueprintPure, Category = "Skill|Shield")
	float GetCurrentShield() const { return CurrentShield; }

	// 인벤토리/상점 등 UI가 열려 있는 동안 공격·스킬 입력만 막는다 (이동은 계속 허용). 잠기는 순간 락온도 풀린다
	void SetGameplayInputLocked(bool bLocked);

	// 인벤토리창/장비창은 함께 열어 둘 수 있으므로, 둘 중 하나라도 열려 있으면 입력 잠금+마우스 커서를 켜고 둘 다 닫히면 게임 입력으로 되돌린다.
	// 창이 열리고 닫힐 때마다 이 함수로 상태를 맞춘다
	void RefreshPanelInputMode();

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

	// 타격마다 피해량이 고정되지 않도록 공격력에 더해지는 무작위 편차의 최대 크기 (2 = -2 ~ +2)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.0"))
	float DamageVariance = 2.f;

	// GetEffectiveAttackPower()에 -DamageVariance ~ +DamageVariance 범위의 정수 난수를 더한 실제 타격 피해량 (최소 1)
	float RollAttackDamage() const;

	// ===== 스킬 C: 강력 가로베기 =====
	// 검에 이펙트가 감긴 채 넓은 호를 그리며 가로로 벤다. 판정은 검이 실제로 지나가는 자리(HitDelay 이후 HitDuration 동안)를
	// 매 프레임 훑고, 몬스터마다 한 번만 들어간다. 애니메이션 노티파이가 필요 없어 몽타주(바이너리)를 건드리지 않는다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|PowerSlash")
	float PowerSlashDamageMultiplier = 3.f;

	// 몽타주 재생 시작부터 검이 휘둘러지기 시작하는(판정과 궤적이 켜지는) 시점까지의 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|PowerSlash")
	float PowerSlashHitDelay = 0.14f;

	// 판정과 궤적이 유지되는 시간(초). 검이 실제로 휘둘러지는 구간에 맞춘다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|PowerSlash")
	float PowerSlashHitDuration = 0.24f;

	// 검 중간과 검 끝, 두 지점을 중심으로 한 판정 구의 반지름. 클수록 넓게 벤다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|PowerSlash")
	float PowerSlashHitRadius = 130.f;

	// 플레이어로부터 이 거리(수평)보다 먼 몬스터는 맞지 않는다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|PowerSlash")
	float PowerSlashMaxRange = 380.f;

	// 몬스터의 KnockbackStrength에 곱해지는 밀어내기 배율 (보스는 밀리지 않음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|PowerSlash")
	float PowerSlashKnockbackScale = 2.f;

	// 일반 공격보다 무겁게 느껴지도록 히트스탑/카메라 셰이크를 키우는 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|PowerSlash")
	float PowerSlashFeelScale = 2.f;

	// 스킬 시작부터 끝까지 검에 붙어 있는 이펙트 (검 오라)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|PowerSlash|FX")
	TObjectPtr<class UNiagaraSystem> PowerSlashSwordFX;

	// 검이 휘둘러지는 동안(판정 구간) 검 끝을 따라 그려지는 궤적 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|PowerSlash|FX")
	TObjectPtr<class UNiagaraSystem> PowerSlashTrailFX;

	// 몬스터를 벨 때마다 그 자리에 터지는 타격 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|PowerSlash|FX")
	TObjectPtr<class UNiagaraSystem> PowerSlashHitFX;

	// 검 오라와 궤적 이펙트의 크기 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|PowerSlash|FX")
	float PowerSlashFXScale = 1.6f;

	// 타격 이펙트의 크기 배율. 한 번에 여러 마리를 벨 수 있어서 검 이펙트보다 작게 잡는다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|PowerSlash|FX")
	float PowerSlashHitFXScale = 0.5f;

	// 타격 이펙트를 켜두는 시간(초). 팩의 NS_Hit_* 는 루프형이라 이 시간이 지나면 강제로 끄지 않으면 몇 초 뒤 다시 터진다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|PowerSlash|FX")
	float PowerSlashHitFXDuration = 0.8f;



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

	// 스킬 C(강력 가로베기): 시작 시 검 오라를 붙이고, HitDelay 뒤에 궤적+판정을 켰다가 HitDuration 뒤에 모두 정리한다
	void BeginPowerSlash();
	void BeginPowerSlashSwing();
	void EndPowerSlashSwing();
	void EndPowerSlash();
	void UpdatePowerSlashHit();

	class UNiagaraComponent* SpawnPowerSlashFX(class UNiagaraSystem* System, FName SocketName);
	void StopPowerSlashFX(TWeakObjectPtr<class UNiagaraComponent>& FXComp);

	bool bPowerSlashSwinging = false;
	FVector LastPowerSlashBladeMid = FVector::ZeroVector;
	FVector LastPowerSlashBladeTop = FVector::ZeroVector;
	FTimerHandle PowerSlashSwingStartTimerHandle;
	FTimerHandle PowerSlashSwingEndTimerHandle;

	// 이번 스윙에서 이미 벤 몬스터. 한 스윙에 한 마리당 한 번만 맞도록 막는다
	TSet<TWeakObjectPtr<AActor>> PowerSlashHitActors;

	TWeakObjectPtr<class UNiagaraComponent> PowerSlashSwordFXComp;
	TWeakObjectPtr<class UNiagaraComponent> PowerSlashTrailFXComp;


	// 무기 판정(WepaonBeginOverlap)과 스킬 C가 함께 쓰는 타격 이펙트 스폰. 맞은 대상에 붙여서 넉백 등으로 대상이 움직여도 따라가게 한다
	void SpawnAttachedHitEffect(TSubclassOf<AEffectDestroy> EffectClass, AActor* Target, const FVector& ImpactPoint, const FVector& ImpactNormal);

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
	void ToggleEquipment();
	void UsePotion();

	// 장비를 끼거나 벗어 최대 체력이 바뀌면 HUD 체력바를 다시 그린다
	UFUNCTION()
	void HandleEquipmentChanged();

	void LockOnKey();
	void LockOnSwitchNext();
	void LockOnSwitchPrev();

	void RollOrDash();

private:
	bool bInvincible = false;
	FTimerHandle InvincibilityTimerHandle;
	FTimerHandle AirDashBrakingTimerHandle;
	float DefaultBrakingDecelerationFalling = 0.f;
	float DefaultFallingLateralFriction = 0.f;

	// 몽타주 시퀀스에 박혀 있는 대시 이펙트 노티파이를 런타임에만 꺼두기 위해 원래 Template을 기억해 두는 목록
	TArray<TPair<TWeakObjectPtr<class UAnimNotify_PlayNiagaraEffect>, TWeakObjectPtr<class UNiagaraSystem>>> SuppressedDashNotifies;

	void SuppressMontageDashEffects();
	void RestoreMontageDashEffects();
	void StartDashEffect(const FVector& Direction);

	void EndInvincibility();
	void RestoreAirBraking();

	// 히트스톱: 게임 전체를 DurationScale * HitStopDuration(실시간 초) 동안 HitStopTimeDilation 배속으로 늦춘다.
	// 타이머 대신 Tick이 실시간 종료 시각을 보고 배속을 되돌리므로, 연타로 여러 번 불려도 가장 늦은 종료 시각만 유지된다
	void PlayHitStop(float DurationScale = 1.f);
	void UpdateTimeDilation();
	void PlayCameraShake(TSubclassOf<class UCameraShakeBase> ShakeClass, float Scale = 1.f);
	void PlayHitRumble(float Intensity, float Duration);

	double HitStopRealEnd = 0.0;

	// 마지막으로 내가 적용한 전역 배속. 다른 시스템(보스 사망 슬로모션 등)이 배속을 바꿔 뒀다면 내가 덮어써 끊지 않는다
	float AppliedTimeDilation = 1.f;

	// 히트스톱이 시작될 때의 전역 배속. 끝나면 1이 아니라 이 값으로 돌려줘서 이미 진행 중이던 슬로모션을 끊지 않는다
	float DilationToRestore = 1.f;

	// 몬스터를 맞힌 직후의 반응. bKnockback=false면 넉백은 호출자가 따로 처리한다(스킬 C)
	void ApplyHitImpact(class AMonster* Monster, const FVector& HitDirection, float FeelScale, bool bFinisher, bool bKnockback);

	// 명중 방향으로 카메라를 밀고 FOV를 좁혔다가 되돌린다 (UpdateCameraPunch가 실시간으로 감쇠)
	void AddCameraPunch(const FVector& WorldDirection, float Scale);
	void UpdateCameraPunch(float RealDelta);

	float BaseCameraFOV = 90.f;
	FVector BaseCameraRelativeLocation = FVector::ZeroVector;
	float CameraPunchFOVCurrent = 0.f;
	FVector CameraPunchOffsetCurrent = FVector::ZeroVector;
	bool bCameraPunchApplied = false;

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
