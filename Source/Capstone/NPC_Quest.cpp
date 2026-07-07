#include "NPC_Quest.h"
#include "UI_Quest.h"

ANPC_Quest::ANPC_Quest()
{}

void ANPC_Quest::Interact_Implementation(AActor * Interactor)
{
    Super::Interact_Implementation(Interactor);

    if (QuestWidgetClass)
    {
        QuestWidget = CreateWidget<UUI_Quest>(GetWorld(), QuestWidgetClass);
        if (QuestWidget)
        {
            QuestWidget->SetQuestData(QuestData);
            QuestWidget->AddToViewport();
        }
    }
}
