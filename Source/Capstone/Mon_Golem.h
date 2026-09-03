#pragma once

#include "Monster.h"
#include "Mon_Golem.generated.h"

UENUM(BlueprintType)
enum class EGolemAttackType : uint8
{
	Left,
	Right,
	Heavy
};

UCLASS()
class CAPSTONE_API AMon_Golem : public AMonster
{
	GENERATED_BODY()

public:
	AMon_Golem();

protected:
	class UMon_GolemAnim* pGolemAnim;

	UPROPERTY(Category = Collision, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* LeftWeaponBox;

	UPROPERTY(Category = Collision, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* RightWeaponBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Setup")
	USkeletalMesh* GolemMeshAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Setup")
	TSubclassOf<UAnimInstance> GolemAnimClass;

	// 골렘 스켈레톤의 실제 손 소켓 이름에 맞게 에디터에서 조정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Setup")
	FName LeftHandSocketName = TEXT("hand_l");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Setup")
	FName RightHandSocketName = TEXT("hand_r");

public:
	// 왼손/오른손 번갈아 공격하는 도중 이 확률로 강공격이 대신 나간다 (0~1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterAttack")
	float CritRate = 0.2f;

	// 강공격 피해량 배율 (iAttack * 이 값)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterAttack")
	float HeavyAttackDamageMultiplier = 2.f;

	EGolemAttackType CurrentAttackType = EGolemAttackType::Left;

private:
	// 강공격은 양손 박스를 동시에 켜므로, 한 번의 공격 판정에서 같은 대상이 두 번 맞지 않도록 기록
	TSet<AActor*> HitActorsThisAttack;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

public:
	// UMon_GolemAnim::InherAttackStartNotify 에서 호출: 공격 종류에 맞는 손의 충돌만 켠다
	void BeginAttack(EGolemAttackType AttackType);
	// UMon_GolemAnim::InherAttackEndNotify 에서 호출: 양손 충돌을 모두 끈다
	void EndAttack();

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
