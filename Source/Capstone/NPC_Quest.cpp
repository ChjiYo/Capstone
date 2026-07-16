#include "NPC_Quest.h"
#include "UI_Quest.h"
#include "Py_Grey.h"
#include "QuestComponent.h"

ANPC_Quest::ANPC_Quest()
{}

void ANPC_Quest::Interact_Implementation(AActor * Interactor)
{

    if (IsValid(QuestWidget)) return;
    if (!QuestChain.IsValidIndex(CurrentChainIndex)) return; // 체인 끝 — 더 줄 퀘스트 없음

    APy_Grey* Player = Cast<APy_Grey>(Interactor);
    if (!Player || !Player->QuestComponent) return;

    UQuestComponent* QC = Player->QuestComponent;
    const FName WantQuestID = QuestChain[CurrentChainIndex].QuestID;

    if (QC->HasActiveQuest())
    {
        const FQuestData& Active = QC->GetActiveQuest();

        if (Active.QuestID != WantQuestID)
            return; // 이 NPC의 현재 체인 퀘스트가 아닌, 다른 진행 중 퀘스트가 있음 (동시 진행은 1개만 허용)

        if (!Active.bIsCompleted)
            return; // 이미 수락해서 진행 중 — 진행도는 좌측 퀘스트 트래커로 확인

        // 방금 완료됨 -> 체인 다음 칸으로
        ++CurrentChainIndex;
        if (!QuestChain.IsValidIndex(CurrentChainIndex)) return;
    }

    Super::Interact_Implementation(Interactor);
    if (QuestWidgetClass)
    {
        QuestWidget = CreateWidget<UUI_Quest>(GetWorld(), QuestWidgetClass);
        if (QuestWidget)
        {
            CurrentInteractor = Interactor;
            QuestWidget->SetQuestData(QuestChain[CurrentChainIndex]);
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
    APy_Grey* Player = Cast<APy_Grey>(CurrentInteractor);
    if (Player && Player->QuestComponent && QuestChain.IsValidIndex(CurrentChainIndex))
    {
        Player->QuestComponent->AcceptQuest(QuestChain[CurrentChainIndex]);
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
