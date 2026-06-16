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

protected:
	virtual void InherAttackStartNotify();
	virtual void InherNextAttackEndNotify();
	virtual void InherAttackEndNotify();


	
};
