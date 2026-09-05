#pragma once

#include "SpawnPoint.h"
#include "CharBase.h"
#include "Monster.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnAttackEndDelegate)
// MonsterID: 처치 퀘스트 판별용 / ExpReward, GoldReward: 처치 시 지급할 보상량
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnMonsterKilled, FName /*MonsterID*/, int32 /*ExpReward*/, int32 /*GoldReward*/)
// bIsBoss인 몬스터가 죽을 때만 추가로 방송 (8단계 엔딩 트리거). APy_Grey가 구독해 지연 후 엔딩 화면을 띄운다
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBossDefeated, FName /*MonsterID*/)

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

	// 몬스터가 죽을 때마다 전역으로 방송. QuestComponent가 이 이벤트를 구독해 처치 퀘스트 진행도를 갱신
	static FOnMonsterKilled OnMonsterKilled;

	static FOnBossDefeated OnBossDefeated;

	// 보스로 취급할지 여부. true면 처치 시 OnMonsterKilled와 별도로 OnBossDefeated도 방송됨 (8단계)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
	bool bIsBoss = false;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	class UBehaviorTree* BehaviorTreeAsset;

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

	// 공격 패턴별 몽타주 (7단계 적응형 AI). 비어 있으면 UBTTask_Attack의 단일 AttackMontage로 폴백해 기존 몬스터는 그대로 동작
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TArray<UAnimMontage*> AttackPatterns;

	// UBTService_PatternAnalysis가 매 공격 전 골라 기록해 두는 인덱스. AWeapon이 명중/회피 결과를 이 인덱스로 귀속시킨다
	int32 CurrentPatternIndex = 0;

	class UUI_HpBar* HpBarWidget;

	FOnAttackEndDelegate OnAttackEnd;
	TArray<FDelegateHandle> AttackEndHandle;

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

	// [TEMP DEBUG] 디졸브 진행 로그를 너무 자주 찍지 않기 위한 누적 타이머. 디버깅 끝나면 제거할 것
	float DebugDissolveLogAccum = 0.f;


public:
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

public:	
	virtual void Tick(float DeltaTime) override;

protected:
	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

public:
	void AttackEnd();
	void Hit();
};
