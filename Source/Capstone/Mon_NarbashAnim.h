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
	virtual void NativeInitializeAnimation();
	virtual void NativeUpdateAnimation(float DeltaSeconds);

public:
	// 보스 브레인이 콤보 스킬을 재생/슬롯 이름을 알아내는 데 쓴다
	UAnimMontage* GetComboMontage() const { return m_pMTGNormalAttack; }

private:
	UFUNCTION()
	void AnimNotify_DashStart();

protected:
	virtual void InherAttackStartNotify();
	virtual void InherNextAttackEndNotify();
	virtual void InherAttackEndNotify();
};
