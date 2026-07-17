#pragma once

#include "GameInfo.h"
#include "GameFramework/Character.h"
#include "CharBase.generated.h"

struct FINFO
{
	int iMaxHp;
	int iHp;
	int iAttack;
};

UENUM(BlueprintType)
enum class EWEAPON : uint8
{
	LHAND   UMETA(DisplayName = "Left Hand"),
	RHAND   UMETA(DisplayName = "Right Hand"),
	HAT     UMETA(DisplayName = "Hat"),
	TOP     UMETA(DisplayName = "Top"),
	BOTTOM  UMETA(DisplayName = "Bottom"),
	ACC     UMETA(DisplayName = "Accessory")
};

UCLASS()
class CAPSTONE_API ACharBase : public ACharacter
{
	GENERATED_BODY()

public:
	ACharBase();

protected:
	UPROPERTY(Category = "Rigging", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TMap<EWEAPON, class AWeapon*> mapWeapon;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Destroy();
protected:
	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);


public:
	class AWeapon* Get_Weapon(EWEAPON eType) { return mapWeapon[eType]; }

	// iMaxHp 상한까지 iHp를 회복시킨다 (포션 등에서 사용)
	void Heal(int32 Amount);

	// 공격력을 증가시킨다 (레벨업, 무기 강화 등에서 사용)
	void AddAttackPower(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Stat")
	int32 GetAttackPower() const { return m_tInfo.iAttack; }

	// 현재 HP 비율(0~1). 적응형 AI가 플레이어의 회복 타이밍을 관측하는 데 사용
	UFUNCTION(BlueprintPure, Category = "Stat")
	float GetHpRatio() const { return m_tInfo.iMaxHp > 0 ? m_tInfo.iHp / static_cast<float>(m_tInfo.iMaxHp) : 0.f; }

protected:
	FINFO m_tInfo;
};
