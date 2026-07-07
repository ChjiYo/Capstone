
#include "UI_Quest.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UUI_Quest::SetQuestData(const FQuestData& InQuestData)
{
	CurrentQuestData = InQuestData;

	if (TitleText)
	{
		TitleText->SetText(CurrentQuestData.QuestTitle);
	}

	if (DescriptionText)
	{
		DescriptionText->SetText(CurrentQuestData.QuestDescription);
	}
}

void UUI_Quest::NativeConstruct()
{
	Super::NativeConstruct();

	if (AcceptButton)
	{
		AcceptButton->OnClicked.AddDynamic(this, &UUI_Quest::OnAcceptClicked);
	}

	if (DeclineButton)
	{
		DeclineButton->OnClicked.AddDynamic(this, &UUI_Quest::OnDeclineClicked);
	}
}

void UUI_Quest::OnAcceptClicked()
{
	RemoveFromParent();
}

void UUI_Quest::OnDeclineClicked()
{
	RemoveFromParent();
}
