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
		else if (InQuestData.HasObjective())
		{
			FText Verb;
			switch (InQuestData.ObjectiveType)
			{
				case EQuestObjectiveType::Kill:    Verb = NSLOCTEXT("UI_QuestTracker", "Kill", "처치"); break;
				case EQuestObjectiveType::Gather:  Verb = NSLOCTEXT("UI_QuestTracker", "Gather", "채집"); break;
				case EQuestObjectiveType::Enhance: Verb = NSLOCTEXT("UI_QuestTracker", "Enhance", "강화"); break;
				default: break;
			}

			ProgressStr = FText::Format(
				NSLOCTEXT("UI_QuestTracker", "ProgressFormat", "{0} {1} / {2}"),
				Verb, InQuestData.CurrentCount, InQuestData.TargetCount);
		}

		ProgressText->SetText(ProgressStr);
	}
}

void UUI_QuestTracker::ClearQuest()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
