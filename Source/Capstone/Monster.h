#pragma once

#include "SpawnPoint.h"
#include "CharBase.h"
#include "CapstoneSFX.h"
#include "Monster.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnAttackEndDelegate)
// MonsterID: 처치 퀘스트 판별용 / ExpReward, GoldReward: 처치 시 지급할 보상량
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnMonsterKilled, FName /*MonsterID*/, int32 /*ExpReward*/, int32 /*GoldReward*/)
// bIsBoss인 몬스터가 죽을 때만 추가로 방송 (8단계 엔딩 트리거). APy_Grey가 구독해 지연 후 엔딩 화면을 띄운다
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBossDefeated, FName /*MonsterID*/)

// 몬스터 종류별 효과음 구성. 몬스터 클래스 생성자가 기본값을 채우고, 블루프린트에서 바꿀 수 있다.
// 기존 애니메이션 노티파이에 이미 소리가 연결된 동작(새/골렘 공격·피격, 고블린 피격)은 None으로 둬서 겹쳐 울리지 않게 한다
USTRUCT(BlueprintType)
struct FMonsterSFXProfile
{
	GENERATED_BODY()

	// 플레이어를 처음 발견했을 때
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX")
	ESFX Alert = ESFX::Mon_Alert_Small;

	// 공격 몽타주가 시작될 때
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX")
	ESFX Attack = ESFX::None;

	// 죽지 않고 맞았을 때
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX")
	ESFX Hit = ESFX::Mon_Hit_Generic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX")
	ESFX Death = ESFX::Mon_Death_Small;

	// 걸을 때 발소리 (None이면 없음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX")
	ESFX Step = ESFX::Mon_Step_Light;

	// 발소리 간격(이동 거리, cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX")
	float StepStride = 160.f;

	// 울음/피격/사망 음성의 피치 배율. 작은 몬스터는 높게, 큰 몬스터는 낮게
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX")
	float VoicePitch = 1.f;

	// 갑옷/바위처럼 단단한 몸이면 플레이어의 검 타격음이 금속성(Hard)으로 난다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX")
	bool bArmored = false;

	// 죽을 때 디졸브(사라지는) 효과음을 깔지 여부. 보스는 끈다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX")
	bool bDissolveSound = true;
};

UCLASS()
class CAPSTONE_API AMonster : public ACharBase
{
	GENERATED_BODY()

public:
	AMonster();
	~AMonster();

public:
	// 처치 퀘스트가 어떤 몬스터를 처치해야 하는지 구분하는 값 (예: "Goblin_Sword", "Goblin_Wizard")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster")
	FName MonsterID;

	// 플레이어 사망 문구("OOO에 의해 사망하셨습니다.") 등 UI에 표시할 몬스터 이름. 몬스터 클래스별 기본값이
	// 생성자에서 채워지며, 같은 클래스의 변종(예: 고블린 마법사)은 블루프린트에서 덮어쓰면 된다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster")
	FText DisplayName;

	// DisplayName이 비어 있으면 MonsterID로 폴백한다
	FText GetDisplayName() const;

	// 몬스터가 죽을 때마다 전역으로 방송. QuestComponent가 이 이벤트를 구독해 처치 퀘스트 진행도를 갱신
	static FOnMonsterKilled OnMonsterKilled;

	static FOnBossDefeated OnBossDefeated;

	// 보스로 취급할지 여부. true면 처치 시 OnMonsterKilled와 별도로 OnBossDefeated도 방송됨 (8단계)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	bool bIsBoss = false;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	class UBehaviorTree* BehaviorTreeAsset;

	// AMonsterAIController가 빙의 시점에 "AI.Subtree.Combat" 슬롯에 주입할 전투 서브트리를 물어보는 훅.
	// 기본은 BP에 지정된 BehaviorTreeAsset이고, 코드로 트리를 조립하는 몬스터(예: 나르바쉬 보스)는 재정의한다
	virtual class UBehaviorTree* GetCombatBehaviorTree(class UBlackboardData* BlackboardAsset) { return BehaviorTreeAsset; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	ASpawnPoint* pSpawnPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* HpBarComponent;

	UPROPERTY(Category = "MonsterAttack", EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float fTraceRange;

	UPROPERTY(Category = "MonsterAttack", EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float fAttackDist;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop Items")
	TArray<TSubclassOf<class AItemBase>> DropItemClasses;

	// 전리품 드롭 확률 (0~1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop Items")
	float DropChance = 1.f;

	// 처치 시 지급되는 경험치/골드. 몬스터 블루프린트별로 다르게 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	int32 ExpReward = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	int32 GoldReward = 10;

	// 피격 시 공격자 반대 방향으로 밀려나는 세기
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterAttack")
	float KnockbackStrength = 300.f;

	// ===== 타격 반동 =====
	// 플레이어에게 맞는 순간 메시가 맞은 방향으로 밀렸다가 제자리로 튕겨 돌아오는 연출 (콜리전/AI에는 영향 없음).
	// 피격 몽타주가 없는 슈퍼아머 몬스터(보스)도 맞는 느낌이 나도록 메시 오프셋만으로 표현한다
	// 몬스터별 반동 세기 배율. 보스처럼 무거운 몬스터는 BP나 생성자에서 낮춘다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel|Recoil")
	float HitRecoilScale = 1.f;

	// 반동 오프셋이 제자리로 돌아오는 속도. 클수록 빨리 튕겨 돌아온다 (1/초, 지수 감쇠)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel|Recoil")
	float HitRecoilReturnSpeed = 14.f;

	// 연타로 반동이 겹쳐도 메시가 이 거리(cm) 이상 밀려나지 않는다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel|Recoil")
	float MaxHitRecoil = 40.f;

	// ===== 락온 =====
	// false면 락온 대상으로 잡히지 않는다 (무적 연출용 몬스터 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	bool bCanBeLockedOn = true;

	// 락온 카메라가 바라보고 마커가 뜨는 지점. 소켓이 지정돼 있고 실제로 존재하면 그 소켓을, 아니면 액터 위치 + 높이 보정을 쓴다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	FName LockOnSocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float LockOnHeightOffset = 0.f;

	// 락온 마커 크기 배율. 덩치 큰 몬스터는 키운다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float LockOnMarkerScale = 1.f;

	// 피격 시 잠깐 번쩍이는 머티리얼 플래시 효과. 몬스터 머티리얼에 이 이름의 스칼라 파라미터가 있어야 실제로 보임
	// (없으면 조용히 무시됨 - 에디터에서 머티리얼에 파라미터를 추가해줘야 함)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel")
	FName HitFlashParamName = TEXT("FlashAmount");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel")
	float HitFlashDuration = 0.15f;

	// 디졸브(서서히 사라짐) 진행 시간(초). 사망 판정과 동시에 시작되므로, 죽는 애니메이션 길이와
	// 비슷하게(혹은 원하는 타이밍에 맞춰) 조절하면 됨
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	float DissolveDuration = 2.f;

	// 디졸브 효과에 쓰이는 머티리얼 스칼라 파라미터 이름 (0=원래대로, 1=완전히 사라짐).
	// HitFlashParamName과 마찬가지로 몬스터 머티리얼에 이 이름의 파라미터가 있어야 실제로 보임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	FName DissolveParamName = TEXT("DissolveAmount");

	// 플레이어 타격 이펙트가 스폰될 소켓 이름 (몬스터 스켈레톤마다 다르므로 몬스터 BP별로 지정).
	// 비워두면(None) 기존처럼 무기가 실제로 닿은 지점에 스폰됨
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel")
	FName HitEffectSocketName = NAME_None;

	// 피격 시 띄울 데미지 숫자 위젯 클래스. 기본값은 위젯 블루프린트 없이 C++로 만든 기본 스타일이고,
	// 폰트 등을 바꾸고 싶으면 UUI_DamageNumber를 부모로 한 WBP를 만들어 여기에 지정하면 됨
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel")
	TSubclassOf<class UUI_DamageNumber> DamageNumberWidgetClass;

	// 데미지 숫자가 뜨는 높이. 캡슐 절반 높이(머리 위치)에 이 값을 더한 지점에서 시작함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFeel")
	float DamageNumberHeightOffset = 20.f;

	// 공격 패턴별 몽타주 (7단계 적응형 AI). 비어 있으면 UBTTask_Attack의 단일 AttackMontage로 폴백해 기존 몬스터는 그대로 동작
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TArray<UAnimMontage*> AttackPatterns;

	// UBTService_PatternAnalysis가 매 공격 전 골라 기록해 두는 인덱스. AWeapon이 명중/회피 결과를 이 인덱스로 귀속시킨다
	int32 CurrentPatternIndex = 0;

	class UUI_HpBar* HpBarWidget;

	FOnAttackEndDelegate OnAttackEnd;
	TArray<FDelegateHandle> AttackEndHandle;

protected:
	// 서브클래스(보스)가 피격 리액션(Hit 몽타주) 없이 피격 플래시만 재생하고 싶을 때 사용
	void TriggerHitFlash() { HitFlashTimeRemaining = HitFlashDuration; }

private:
	bool bHit;

	// 첫 Tick에서 스프링암 카메라 콜리전 무시 설정을 한 번만 적용했는지 여부
	bool bAppliedCameraCollisionIgnore = false;

	UPROPERTY()
	TArray<class UMaterialInstanceDynamic*> HitFlashMaterials;

	float HitFlashTimeRemaining = 0.f;

	void UpdateHitFlash(float DeltaTime);

	// 디졸브 진행 중인지 여부. 사망 판정과 동시에 true가 되어 데스 모션과 디졸브가 함께 재생되고,
	// DissolveDuration이 지나면 실제로 Destroy한다
	bool bDissolving = false;
	float DissolveTimeElapsed = 0.f;

	void UpdateDeathSequence(float DeltaTime);

	// 탐지 범위(fTraceRange) 안에 플레이어가 들어와 AI가 타겟을 잡았을 때만 HP바를 보이게 한다
	void UpdateHpBarVisibility();

	// 플레이어 화면에 데미지 숫자를 띄운다 (포물선을 그리며 떨어지는 UUI_DamageNumber)
	void ShowDamageNumber(float Damage);

	// [TEMP DEBUG] 디졸브 진행 로그를 너무 자주 찍지 않기 위한 누적 타이머. 디버깅 끝나면 제거할 것
	float DebugDissolveLogAccum = 0.f;


	// 메시 반동 (ApplyHitRecoil이 목표를 쌓고 Tick이 감쇠시키며 메시 상대 위치에 반영)
	FVector MeshBaseRelativeLocation = FVector::ZeroVector;
	FVector HitRecoilOffset = FVector::ZeroVector;
	bool bRecoilApplied = false;

	void UpdateHitRecoil(float DeltaTime);

	bool bLockedOn = false;


public:
	// 락온 카메라/마커가 겨냥하는 월드 좌표
	FVector GetLockOnPoint() const;

	// ULockOnComponent가 대상이 될 때/풀릴 때 호출. 락온된 동안은 탐지 여부와 무관하게 HP바를 보여준다
	void SetLockedOn(bool bLocked) { bLockedOn = bLocked; }

	bool IsLockedOn() const { return bLockedOn; }

	// 맞은 방향(월드, 수평 성분만 사용)으로 Distance(cm)만큼 메시를 밀어낸다. 이후 Tick에서 서서히 제자리로 돌아온다
	void ApplyHitRecoil(const FVector& HitDirection, float Distance);

	template<typename T>
	void AddAttackEndDelegate(T* pObj, void(T::* pFunc)())
	{
		FDelegateHandle handle = OnAttackEnd.AddUObject(pObj, pFunc);

		AttackEndHandle.Add(handle);
	}

	void ClearAttackEndDelegate();

public:
	const float& GetTraceRange() const
	{
		return fTraceRange;
	}

	const float& GetAttackDist() const
	{
		return fAttackDist;
	}

	const bool& GetHit() const
	{
		return bHit;
	}

public:
	void SetHit(bool _Hit)
	{
		bHit = _Hit;
	}

public:
	void SetSpawnPoint(ASpawnPoint* _Point)
	{
		pSpawnPoint = _Point;
	}

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;

protected:
	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX")
	FMonsterSFXProfile SFXProfile;

	// AI가 플레이어를 처음 발견했을 때 (UMonsterAIController::SetTarget). 짧은 시간 안의 재발견은 무시한다
	void PlayAlertSFX();

	// 공격 몽타주가 시작될 때 (UBTTask_Attack)
	void PlayAttackSFX();

	// 플레이어의 검이 이 몬스터를 때렸을 때 들려줄 타격음 (갑옷이면 금속성)
	ESFX GetPlayerHitSFX() const { return SFXProfile.bArmored ? ESFX::Player_HitHard : ESFX::Player_HitFlesh; }

private:
	void UpdateFootsteps();

	FVector LastFootstepLocation = FVector::ZeroVector;
	float FootstepDistance = 0.f;
	double LastAlertTime = -1000.0;

public:
	void AttackEnd();

	// 보스처럼 슈퍼아머(피격 시 동작이 끊기지 않음)가 필요한 몬스터가 재정의한다
	virtual void Hit();
};
