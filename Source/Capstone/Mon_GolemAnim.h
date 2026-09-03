#pragma once

#include "MonsterAnim.h"
#include "Mon_GolemAnim.generated.h"

UCLASS()
class CAPSTONE_API UMon_GolemAnim : public UMonsterAnim
{
	GENERATED_BODY()

public:
	UMon_GolemAnim();

protected:
	virtual void InherAttackStartNotify() override;
	virtual void InherNextAttackEndNotify() override;
	virtual void InherAttackEndNotify() override;

public:
	// m_pMTGNormalAttack 몽타주 안에 있어야 하는 섹션 이름. 실제 몽타주에서 만든 섹션 이름과 맞춰줄 것
	UPROPERTY(EditAnywhere, Category = "Attack Sections")
	FName LeftAttackSection = TEXT("AttackLeft");

	UPROPERTY(EditAnywhere, Category = "Attack Sections")
	FName RightAttackSection = TEXT("AttackRight");

	UPROPERTY(EditAnywhere, Category = "Attack Sections")
	FName HeavyAttackSection = TEXT("AttackHeavy");

private:
	// 다음 정타가 왼손/오른손 중 어느 쪽 차례인지 추적. 강공격이 끼어들어도 이 값은 바뀌지 않아
	// 교대 순서(왼->오른->왼->오른)가 그대로 유지된다
	bool bLastAttackWasLeft = false;
};
