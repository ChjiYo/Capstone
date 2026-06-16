#pragma once

#include "MonsterAnim.h"
#include "Mon_NarbashAnim.generated.h"

UCLASS()
class CAPSTONE_API UMon_NarbashAnim : public UMonsterAnim
{
	GENERATED_BODY()

public:
	UMon_NarbashAnim();


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
	void ChangeAnim(EBaseAnim eAnim);
	virtual void NormalAttack(bool bAttack = true) override;

public:
	void SkillSlotA();
	void SkillSlotB();
	void SkillSlotC();


private:
	virtual void AnimNotify_AttackStart() override;
	virtual void AnimNotify_NextAttackEnd() override;
	virtual void AnimNotify_AttackEnd() override;	
	virtual void AnimNotify_SkillAStart() override;

protected:
	virtual void InherAttackStartNotify();
	virtual void InherNextAttackEndNotify();
	virtual void InherAttackEndNotify();
	virtual void InherSkillAStartNotify();
};
