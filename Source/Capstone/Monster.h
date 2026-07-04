#pragma once

#include "SpawnPoint.h"
#include "CharBase.h"
#include "Monster.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnAttackEndDelegate)

UCLASS()
class CAPSTONE_API AMonster : public ACharBase
{
	GENERATED_BODY()

public:
	AMonster();
	~AMonster();

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
