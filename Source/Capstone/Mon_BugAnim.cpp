#include "Mon_BugAnim.h"
#include "Mon_Bug.h"

// 공격 몽타주(m_pMTGNormalAttack)는 섹션 하나만 있으면 되고(콤보 없음),
// 피격 몽타주(m_pMTGHit)도 반드시 채워야 한다 - 비워두면 피격 후 다시는 공격하지 못하는
// 버그(bHit이 영원히 true로 고착)가 생긴다. 두 몽타주 모두 이 클래스를 부모로 하는
// 애니메이션 블루프린트(예: AP_Buff_Blue)의 클래스 디폴트에서 지정한다.
UMon_BugAnim::UMon_BugAnim()
{
	eAnimType = EBaseAnim::Idle;
	eDir = EDir::Front;

	CurrentCombo = 1;
	NextAttackMax = 1;
	bIsComboInputOn = true;
}

void UMon_BugAnim::InherAttackStartNotify()
{
	AMon_Bug* pOwner = Cast<AMon_Bug>(TryGetPawnOwner());

	if (IsValid(pOwner))
		pOwner->EnableWeaponCollision(true);
}

void UMon_BugAnim::InherNextAttackEndNotify()
{
	// NextAttackMax가 1이라 항상 콤보 없이 종료된다
	bIsComboInputOn = false;
}

void UMon_BugAnim::InherAttackEndNotify()
{
	CurrentCombo = 1;
	bIsComboInputOn = true;

	AMonster* pOwner = Cast<AMonster>(TryGetPawnOwner());

	if (IsValid(pOwner))
		pOwner->AttackEnd();

	AMon_Bug* pBugOwner = Cast<AMon_Bug>(TryGetPawnOwner());

	if (IsValid(pBugOwner))
		pBugOwner->EnableWeaponCollision(false);
}
