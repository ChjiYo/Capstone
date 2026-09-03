#pragma once

#include "MonsterAnim.h"
#include "Mon_BugAnim.generated.h"

UCLASS()
class CAPSTONE_API UMon_BugAnim : public UMonsterAnim
{
	GENERATED_BODY()

public:
	UMon_BugAnim();

protected:
	virtual void InherAttackStartNotify() override;
	virtual void InherNextAttackEndNotify() override;
	virtual void InherAttackEndNotify() override;
};
