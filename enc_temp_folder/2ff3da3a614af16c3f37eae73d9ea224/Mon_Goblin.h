#pragma once

#include "Monster.h"
#include "Mon_Goblin.generated.h"

UCLASS()
class CAPSTONE_API AMon_Goblin: public AMonster
{
	GENERATED_BODY()

public:
	AMon_Goblin();

protected:
	class UMon_GoblinAnim* pGoblinAnim;

	UPROPERTY(Category = Collision, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* WeaponBox;

	UPROPERTY(Category = Collision, VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FString strWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Setup")
	USkeletalMesh* GoblinMeshAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Setup")
	TSubclassOf<UAnimInstance> GoblinAnimClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Setup")
	FString WeaponMeshPath;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

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
