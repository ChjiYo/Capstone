#include "UI_BossHpBar.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UUI_BossHpBar::SetBossHp(float fPercent)
{
	if (IsValid(HpBar))
	{
		HpBar->SetPercent(fPercent);
	}
}

void UUI_BossHpBar::SetPhase(int32 Phase)
{
	if (IsValid(PhaseText))
	{
		PhaseText->SetText(FText::Format(NSLOCTEXT("UI_BossHpBar", "PhaseFormat", "Phase {0}"), Phase));
	}
}
