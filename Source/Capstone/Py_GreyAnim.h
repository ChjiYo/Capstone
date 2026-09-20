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

	// 스킬 C(강력 가로베기) 모션. 지정돼 있으면 m_pMTGSkillSlotC 대신 이 시퀀스를 SlotA 슬롯에 동적 몽타주로 재생한다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimSequence* m_pSkillCSequence;

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
