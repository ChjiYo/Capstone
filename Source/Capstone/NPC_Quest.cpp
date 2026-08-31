#include "NPC_Quest.h"
#include "UI_Quest.h"
#include "Py_Grey.h"
#include "QuestComponent.h"
#include "StoryComponent.h"
#include "StoryStateSubsystem.h"
#include "Kismet/GameplayStatics.h"

ANPC_Quest::ANPC_Quest()
{
    NPCTypeTitle = TEXT("퀘스트");
}

void ANPC_Quest::Interact_Implementation(AActor * Interactor)
{

    if (IsValid(QuestWidget)) return;
    if (!QuestChain.IsValidIndex(CurrentChainIndex)) return; // 체인 끝 — 더 줄 퀘스트 없음

    APy_Grey* Player = Cast<APy_Grey>(Interactor);
    if (!Player || !Player->QuestComponent) return;

    UStoryComponent* Story = Player->StoryComponent;
    if (Story && Story->IsPlaying()) return; // 대사 재생 중 재진입 차단

    UQuestComponent* QC = Player->QuestComponent;
    const FName WantQuestID = QuestChain[CurrentChainIndex].QuestID;

    if (QC->HasActiveQuest())
    {
        const FQuestData& Active = QC->GetActiveQuest();

        if (Active.QuestID != WantQuestID)
            return; // 이 NPC의 현재 체인 퀘스트가 아닌, 다른 진행 중 퀘스트가 있음 (동시 진행은 1개만 허용)

        if (!Active.bIsCompleted)
        {
            // 진행 중 — 대사만 한 줄, 수락창은 열지 않는다 (기존 동작 유지)
            const TArray<FName>& ProgLines = QuestChain[CurrentChainIndex].InProgressDialogueLines;
            if (Story && ProgLines.Num() > 0)
                Story->PlayDialogue(ProgLines);
            return;
        }

        // 방금 완료됨 -> 완료 대사 후 체인 다음 칸으로
        CurrentInteractor = Interactor;
        const TArray<FName> DoneLines = QuestChain[CurrentChainIndex].CompleteDialogueLines;
        ++CurrentChainIndex;

        if (Story && DoneLines.Num() > 0)
        {
            Story->PlayDialogue(DoneLines, true, FSimpleDelegate::CreateUObject(this, &ANPC_Quest::BeginNextQuestIntro));
            return;
        }
        BeginNextQuestIntro();
        return;
    }

    CurrentInteractor = Interactor;
    BeginNextQuestIntro();
}

void ANPC_Quest::BeginNextQuestIntro()
{
    if (!QuestChain.IsValidIndex(CurrentChainIndex))
    {
        CurrentInteractor = nullptr; // 체인 끝 — 더 줄 퀘스트 없음
        return;
    }

    APy_Grey* Player = Cast<APy_Grey>(CurrentInteractor);
    UStoryComponent* Story = Player ? Player->StoryComponent : nullptr;
    const TArray<FName>& IntroLines = QuestChain[CurrentChainIndex].IntroDialogueLines;

    if (Story && IntroLines.Num() > 0)
    {
        Story->PlayDialogue(IntroLines, true, FSimpleDelegate::CreateUObject(this, &ANPC_Quest::OpenQuestWidget));
        return;
    }
    OpenQuestWidget();
}

void ANPC_Quest::OpenQuestWidget()
{
    if (!QuestChain.IsValidIndex(CurrentChainIndex) || !CurrentInteractor) return;

    Super::Interact_Implementation(CurrentInteractor);
    if (QuestWidgetClass)
    {
        QuestWidget = CreateWidget<UUI_Quest>(GetWorld(), QuestWidgetClass);
        if (QuestWidget)
        {
            QuestWidget->SetQuestData(QuestChain[CurrentChainIndex]);
            QuestWidget->OnQuestAccepted.AddDynamic(this, &ANPC_Quest::HandleQuestAccepted);
            QuestWidget->OnQuestDeclined.AddDynamic(this, &ANPC_Quest::HandleQuestDeclined);
            QuestWidget->AddToViewport();

            APlayerController* PC = Cast<APlayerController>(Cast<APawn>(CurrentInteractor)->GetController());
            if (PC)
            {
                PC->bShowMouseCursor = true;
                PC->SetInputMode(FInputModeGameAndUI());
            }
        }
    }
}

void ANPC_Quest::GrantCurrentQuestFor(AActor* Interactor)
{
    APy_Grey* Player = Cast<APy_Grey>(Interactor);
    if (!Player || !Player->QuestComponent)
        return;

    UQuestComponent* QC = Player->QuestComponent;

    // 현재 체인 퀘스트를 이미 완료한 채로 남아있으면(NPC를 직접 재방문하지 않고
    // 트리거로만 스토리가 진행되는 경우, Interact_Implementation의 체인 진행 로직을 못 타므로)
    // 여기서 대신 다음 체인으로 넘긴다
    if (QC->HasActiveQuest() && QuestChain.IsValidIndex(CurrentChainIndex)
        && QC->GetActiveQuest().QuestID == QuestChain[CurrentChainIndex].QuestID
        && QC->GetActiveQuest().bIsCompleted)
    {
        ++CurrentChainIndex;
    }

    if (!QuestChain.IsValidIndex(CurrentChainIndex))
        return;

    const FQuestData& Q = QuestChain[CurrentChainIndex];

    // 이미 받은(진행 중이거나 완료된) 퀘스트면 트리거 재진입 등으로 다시 지급하지 않는다
    if (QC->HasActiveQuest() && QC->GetActiveQuest().QuestID == Q.QuestID)
        return;

    QC->AcceptQuest(Q);

    if (Player->StoryComponent && !Q.ChapterLabel.IsEmpty())
    {
        Player->StoryComponent->ShowChapterTitle(Q.ChapterLabel, Q.QuestTitle);

        if (UStoryStateSubsystem* SS = UGameplayStatics::GetGameInstance(this)->GetSubsystem<UStoryStateSubsystem>())
        {
            SS->SetChapter(CurrentChainIndex + 1);
        }
    }
}

void ANPC_Quest::HandleQuestAccepted()
{
    GrantCurrentQuestFor(CurrentInteractor);

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
