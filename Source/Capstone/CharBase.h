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

USTRUCT(BlueprintType)
struct FRigging
{
	GENERATED_BODY()

	UPROPERTY(Category = "LeftWeapon", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class AWeapon* m_pLeftWeapon;

	UPROPERTY(Category = "RightWeapon", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class AWeapon* m_pRightWeapon;

	UPROPERTY(Category = "Hat", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class AWeapon* m_pHat;

	UPROPERTY(Category = "Top", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class AWeapon* m_pTop;

	UPROPERTY(Category = "Bottom", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class AWeapon* m_pBottom;

	UPROPERTY(Category = "ACC", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class AWeapon* m_pACC;
};

UCLASS()
class CAPSTONE_API ACharBase : public ACharacter
{
	GENERATED_BODY()



public:
	ACharBase();

protected:
	UPROPERTY(Category = "Rigging", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FRigging m_tRigging;
		
protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);


public:
	const FRigging Get_RiggingInfo() { return m_tRigging; }
	
protected:
	FINFO m_tInfo;
	FString CharacterName;

};
