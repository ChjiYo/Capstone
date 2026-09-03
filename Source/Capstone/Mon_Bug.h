#pragma once

#include "Monster.h"
#include "Mon_Bug.generated.h"

// Buff_Blue: 공격 모션 1개만 사용하는 단발형 몬스터
UCLASS()
class CAPSTONE_API AMon_Bug : public AMonster
{
	GENERATED_BODY()

public:
	AMon_Bug();

protected:
	class UMon_BugAnim* pBugAnim;

	UPROPERTY(Category = Collision, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* WeaponBox;

	// 블루프린트(BP_Buff_Blue)에서 메시/애님 블루프린트를 지정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Setup")
	USkeletalMesh* BugMeshAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Setup")
	TSubclassOf<UAnimInstance> BugAnimClass;

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
