#pragma once

#include "Monster.h"
#include "Mon_Bird.generated.h"

// Buff_Black: 공격 모션 2개를 이어서 사용하는 콤보형 몬스터
UCLASS()
class CAPSTONE_API AMon_Bird : public AMonster
{
	GENERATED_BODY()

public:
	AMon_Bird();

protected:
	class UMon_BirdAnim* pBirdAnim;

	UPROPERTY(Category = Collision, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* WeaponBox;

	// 블루프린트(BP_Buff_Black)에서 메시/애님 블루프린트를 지정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Setup")
	USkeletalMesh* BirdMeshAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Setup")
	TSubclassOf<UAnimInstance> BirdAnimClass;

	// 실제 스켈레톤의 공격 판정 소켓 이름에 맞게 에디터에서 조정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Setup")
	FName WeaponSocketName = TEXT("Attack_Socket");

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

public:
	void EnableWeaponCollision(bool bEnable);

	UFUNCTION()
	void WepaonBeginOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void WeaponEndOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
