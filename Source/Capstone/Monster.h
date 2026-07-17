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
