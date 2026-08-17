#include "UI_ChapterTitle.h"
#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h"

void UUI_ChapterTitle::NativeConstruct()
{
	Super::NativeConstruct();

	if (FadeAnim)
	{
		PlayAnimation(FadeAnim);
	}
}

void UUI_ChapterTitle::SetChapter(const FText& InLabel, const FText& InTitle)
{
	if (ChapterLabelText)
	{
		ChapterLabelText->SetText(InLabel);
	}

	if (ChapterTitleText)
	{
		ChapterTitleText->SetText(InTitle);
	}
}
