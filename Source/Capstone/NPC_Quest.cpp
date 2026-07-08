#include "NPC_Quest.h"
#include "UI_Quest.h"
#include "Py_Grey.h"
#include "InventoryComponent.h"
#include "QuestComponent.h"

ANPC_Quest::ANPC_Quest()
{}

void ANPC_Quest::Interact_Implementation(AActor * Interactor)
{

    if (IsValid(QuestWidget) || QuestData.bIsCompleted)
        return;

    Super::Interact_Implementation(Interactor);
    if (QuestWidgetClass)
    {
        QuestWidget = CreateWidget<UUI_Quest>(GetWorld(), QuestWidgetClass);
        if (QuestWidget)
        {
            CurrentInteractor = Interactor;

            QuestWidget->SetQuestData(QuestData);
            QuestWidget->OnQuestAccepted.AddDynamic(this, &ANPC_Quest::HandleQuestAccepted);
            QuestWidget->OnQuestDeclined.AddDynamic(this, &ANPC_Quest::HandleQuestDeclined);
            QuestWidget->AddToViewport();

            APlayerController* PC = Cast<APlayerController>(Cast<APawn>(Interactor)->GetController());
            if (PC)
            {
                PC->bShowMouseCursor = true;
                PC->SetInputMode(FInputModeGameAndUI());
            }
        }
    }
}

void ANPC_Quest::HandleQuestAccepted()
{
    // NPC 입장에서는 "이미 지급함" 표시. 처치 목표가 아직 안 끝났어도 재상호작용으로
    // 다시 받아서 진행도가 리셋되면 안 되므로 여기서 바로 완료 처리한다
    QuestData.bIsCompleted = true;

    APy_Grey* Player = Cast<APy_Grey>(CurrentInteractor);

    if (Player && QuestData.HasKillObjective())
    {
        // 처치 목표가 있는 퀘스트는 QuestComponent가 진행도를 추적하고, 목표 달성 시 보상을 지급한다
        if (Player->QuestComponent)
        {
            Player->QuestComponent->AcceptQuest(QuestData);
        }
    }
    else if (Player && Player->InventoryComponent && !QuestData.Reward.RewardItemID.IsNone())
    {
        // 처치 목표가 없는(대화/전달형) 퀘스트는 수락 즉시 보상 지급
        for (int32 i = 0; i < QuestData.Reward.RewardItemCount; ++i)
        {
            Player->InventoryComponent->AddItem(QuestData.Reward.RewardItemID);
        }
    }

    RestoreInputMode();
    QuestWidget = nullptr;
    CurrentInteractor = nullptr;
}

void ANPC_Quest::HandleQuestDeclined()
{
    RestoreInputMode();
    QuestWidget = nullptr;
    CurrentInteractor = nullptr;
}

void ANPC_Quest::RestoreInputMode()
{
    APawn* Pawn = Cast<APawn>(CurrentInteractor);
    if (Pawn)
    {
        APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
        if (PC)
        {
            PC->bShowMouseCursor = false;
            PC->SetInputMode(FInputModeGameOnly());
        }
    }
}
