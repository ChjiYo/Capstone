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
	void ChangeAnim(EBaseAnim eAnim);

private:
	virtual void AnimNotify_AttackStart() override;
	virtual void AnimNotify_NextAttackEnd() override;
	virtual void AnimNotify_AttackEnd() override;	

protected:
	virtual void InherAttackStartNotify();
	virtual void InherNextAttackEndNotify();
	virtual void InherAttackEndNotify();
};
