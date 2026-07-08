#include "UI_ExpBar.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UUI_ExpBar::SetExp(int32 CurrentExp, int32 RequiredExp, int32 Level)
{
	if (ExpBar)
	{
		float fPercent = RequiredExp > 0 ? static_cast<float>(CurrentExp) / static_cast<float>(RequiredExp) : 0.f;
		ExpBar->SetPercent(fPercent);
	}

	if (LevelText)
	{
		LevelText->SetText(FText::FromString(FString::Printf(TEXT("Lv. %d"), Level)));
	}

	if (ExpText)
	{
		ExpText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), CurrentExp, RequiredExp)));
	}
}
