#pragma once

#include "MonsterAnim.h"
#include "Mon_BirdAnim.generated.h"

UCLASS()
class CAPSTONE_API UMon_BirdAnim : public UMonsterAnim
{
	GENERATED_BODY()

public:
	UMon_BirdAnim();

protected:
	virtual void InherAttackStartNotify() override;
	virtual void InherNextAttackEndNotify() override;
	virtual void InherAttackEndNotify() override;
};
