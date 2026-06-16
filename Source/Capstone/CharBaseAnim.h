#pragma once

#include "GameInfo.h"
#include "Animation/AnimInstance.h"
#include "CharBaseAnim.generated.h"

UENUM(BlueprintType)
enum class EBaseAnim : uint8
{
	Idle,
	Run,
	Hit,
	Attack,
	AttackDiscovery,
	Jump,
	Skill,
	Death
};


UCLASS()
class CAPSTONE_API UCharBaseAnim : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	UCharBaseAnim();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	EBaseAnim eAnimType;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	EDir eDir;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	bool bMoveStop;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool IsGround;
	
protected:
	bool bIsComboInputOn;
	uint8 NextAttackMax;
	int32 CurrentCombo;

protected:
	virtual void NativeInitializeAnimation();
	virtual void NativeUpdateAnimation(float DeltaSeconds);

public:
	virtual void NormalAttack() {};
	void JumpKey();

public:
	void SetMoveDir(EDir _eDir)
	{
		eDir = _eDir;
	}

private:
	UFUNCTION()
	void AnimNotify_MoveStop();

	UFUNCTION()
	virtual void AnimNotify_AttackStart();

	UFUNCTION()
	virtual void AnimNotify_NextAttackEnd();

	UFUNCTION()
	virtual void AnimNotify_AttackEnd();

	UFUNCTION()
	virtual void AnimNotify_ReturnIdle();

	UFUNCTION()
	virtual void AnimNotify_SkillAStart();



protected:
	virtual void InherMoveStopNotify() {};
	virtual void InherAttackStartNotify() {};
	virtual void InherNextAttackEndNotify() {};
	virtual void InherAttackEndNotify() {};
	virtual void InherReturnIdleNotify() {};
	virtual void InherSkillAStartNotify() {};

public:
	void SetAnimType(EBaseAnim _eType)
	{
		eAnimType = _eType;
	}


};
