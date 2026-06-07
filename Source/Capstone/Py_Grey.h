#pragma once

#include "CharBase.h"
#include "Py_Grey.generated.h"

UCLASS()
class CAPSTONE_API APy_Grey : public ACharBase
{
	GENERATED_BODY()

public:
	APy_Grey();


protected:
	UPROPERTY(Category = Camera, VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* pCamera;

	UPROPERTY(Category = Arm, VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* pArm;

	UPROPERTY(Category = Collision, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* WeaponBox;

	class UPy_GreyAnim* pGreyAnim;
protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	void MoveFront(float _fScale);
	void MoveSide(float _fScale);
	void RotationZ(float _fScale);

private:
	void JumpKey();
	void NormalAttack();
	void SkillSlotA();
	void SkillSlotB();
	void SkillSlotC();

public:
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
