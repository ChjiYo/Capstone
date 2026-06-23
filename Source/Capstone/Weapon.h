#pragma once

#include "GameInfo.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

USTRUCT(BlueprintType)
struct FWeaponData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float BaseDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FVector CollisionBoxSize = FVector(10.f, 10.f, 40.f); 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName CollisionType = TEXT("MonsterAttack");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName StartSocket= TEXT("StartSocket");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName EndSocket = TEXT("EndSocket ");

};

UCLASS()
class CAPSTONE_API AWeapon : public AActor
{
	GENERATED_BODY()
		
public:	
	AWeapon();

protected:
	UPROPERTY(Category = "Mesh", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh;

	UPROPERTY(Category = Collision, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* CollisionBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Data")
	FWeaponData WeaponInfo;

public:
	FORCEINLINE class USkeletalMeshComponent* GetWeaponMesh() const { return Mesh; }

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

public:
	void LoadMesh(const FString& strPath);
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

};
