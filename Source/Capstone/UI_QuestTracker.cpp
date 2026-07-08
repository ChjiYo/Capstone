#include "UI_QuestTracker.h"
#include "Components/TextBlock.h"

void UUI_QuestTracker::UpdateQuest(const FQuestData& InQuestData)
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (TitleText)
	{
		TitleText->SetText(InQuestData.QuestTitle);
	}

	if (DescriptionText)
	{
		DescriptionText->SetText(InQuestData.QuestDescription);
	}

	if (ProgressText)
	{
		FText ProgressStr;

		if (InQuestData.bIsCompleted)
		{
			ProgressStr = NSLOCTEXT("UI_QuestTracker", "Completed", "완료!");
		}
		else if (InQuestData.HasKillObjective())
		{
			ProgressStr = FText::Format(
				NSLOCTEXT("UI_QuestTracker", "ProgressFormat", "처치 {0} / {1}"),
				InQuestData.CurrentKillCount,
				InQuestData.KillTargetCount);
		}

		ProgressText->SetText(ProgressStr);
	}
}

void UUI_QuestTracker::ClearQuest()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
