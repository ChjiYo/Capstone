#pragma once

#include "GameInfo.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

UCLASS()
class CAPSTONE_API AWeapon : public AActor
{
	GENERATED_BODY()


	struct FWeaponAtt
	{
		int iSword = 8;
	};

	
public:	
	AWeapon();

protected:
	UPROPERTY(Category = "Mesh", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TEnumAsByte<ECollisionChannel> AttackTraceChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BaseDamage;

private:
	bool bIsAttacking = false;

	UPROPERTY()
	TArray<AActor*> DamagedActors;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	

public:
	void SetAttackState(bool bActive);

public:
	void LoadMesh(const FString& strPath);
	void TraceWeapon();

private:
	FWeaponAtt m_tWeaponAtt;

};
