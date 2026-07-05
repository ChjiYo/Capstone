#pragma once

#include "CoreMinimal.h"
#include "MonsterAnim.h"
#include "Mon_LarvaeAnim.generated.h"

UCLASS()
class CAPSTONE_API UMon_LarvaeAnim : public UMonsterAnim
{
	GENERATED_BODY()

public:
	UMon_LarvaeAnim();

protected:
	virtual void NativeInitializeAnimation();
	virtual void NativeUpdateAnimation(float DeltaSeconds);


protected:
	virtual void InherAttackStartNotify();
	virtual void InherNextAttackEndNotify();
	virtual void InherAttackEndNotify();
};
