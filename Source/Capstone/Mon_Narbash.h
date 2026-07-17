#pragma once

#include "Monster.h"
#include "Mon_Narbash.generated.h"

UCLASS()
class CAPSTONE_API AMon_Narbash : public AMonster
{
	GENERATED_BODY()

public:
	AMon_Narbash();

protected:
	class UMon_NarbashAnim* pNarbashAnim;

	UPROPERTY(Category = Collision, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* LeftWeaponBox;

	UPROPERTY(Category = Collision, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* RightWeaponBox;

	UPROPERTY(Category = Collision, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USphereComponent* DashSphere;



protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;

protected:
	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

public:
	void EnableWeaponCollision(bool bEnable);
	void ExecutePhysicalDash();

public:
	// HP 비율이 이 값 이하로 떨어지면 2페이즈로 전환 (8단계 보스전)
	UPROPERTY(EditAnywhere, Category = "Boss")
	float Phase2HpRatio = 0.5f;

	// 2페이즈 전용 공격 패턴. 전환 시 AttackPatterns를 이걸로 교체 (7단계 학습은 계속 적용됨)
	UPROPERTY(EditAnywhere, Category = "Boss")
	TArray<UAnimMontage*> Phase2AttackPatterns;

	UPROPERTY(EditAnywhere, Category = "Boss")
	float Phase2SpeedMultiplier = 1.3f;

	UPROPERTY(EditAnywhere, Category = "Boss")
	UAnimMontage* PhaseTransitionMontage;

	UPROPERTY(EditAnywhere, Category = "Boss")
	TSubclassOf<class UCameraShakeBase> PhaseTransitionCameraShake;

	// 전환 연출 동안 무적 처리할 시간(초)
	UPROPERTY(EditAnywhere, Category = "Boss")
	float PhaseTransitionInvincibleDuration = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UUI_BossHpBar> BossHpBarClass;

	int32 CurrentPhase = 1;

private:
	bool bPhaseTransitioning = false;
	FTimerHandle PhaseTransitionTimerHandle;

	UPROPERTY()
	class UUI_BossHpBar* BossHpBarWidget = nullptr;

	void EnterPhase2();
	void EndPhaseTransition();

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


	UFUNCTION()
	void OnCollisionOverlap(UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor, UPrimitiveComponent* OtherComp, int32
		OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);
};
