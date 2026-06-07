#pragma once

#include "CharBaseAnim.h"
#include "Py_GreyAnim.generated.h"

UCLASS()
class CAPSTONE_API UPy_GreyAnim : public UCharBaseAnim
{
	GENERATED_BODY()

public:
	UPy_GreyAnim();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* m_pMTGSkillSlotA;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* m_pMTGSkillSlotB;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* m_pMTGSkillSlotC;

protected:
	virtual void NativeInitializeAnimation();
	virtual void NativeUpdateAnimation(float DeltaSeconds);

public:
	void SkillSlotA();
	void SkillSlotB();
	void SkillSlotC();

protected:
	virtual void InherMoveStopNotify();
	virtual void InherAttackStartNotify();
	virtual void InherNextAttackEndNotify();
	virtual void InherAttackEndNotify();
	virtual void InherReturnIdleNotify();

	
};
