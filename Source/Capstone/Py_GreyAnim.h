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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* m_pMTGNormalAttack;

protected:
	virtual void NativeInitializeAnimation();
	virtual void NativeUpdateAnimation(float DeltaSeconds);

public:
	void SkillSlotA();
	void SkillSlotB();
	void SkillSlotC();

public:
	virtual void NormalAttack() override;

protected:
	virtual void InherMoveStopNotify() override;
	virtual void InherAttackStartNotify() override;
	virtual void InherNextAttackEndNotify() override;
	virtual void InherAttackEndNotify() override;
	virtual void InherReturnIdleNotify() override;

	
};
