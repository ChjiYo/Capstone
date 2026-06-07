#pragma once

#include "MonsterAnim.h"
#include "Mon_GoblinAnim.generated.h"

UCLASS()
class CAPSTONE_API UMon_GoblinAnim : public UMonsterAnim
{
	GENERATED_BODY()

public:
	UMon_GoblinAnim();

protected:
	virtual void NativeInitializeAnimation();
	virtual void NativeUpdateAnimation(float DeltaSeconds);

public:
	void ChangeAnim(EBaseAnim eAnim);

private:
	virtual void AnimNotify_AttackStart() override;
	virtual void AnimNotify_AttackEnd() override;


protected:
	virtual void InherAttackStartNotify();
	virtual void InherAttackEndNotify();


	
};
