#include "StoryComponent.h"
#include "DialogueData.h"
#include "UI_Dialogue.h"
#include "UI_ChapterTitle.h"
#include "StoryStateSubsystem.h"
#include "Py_Grey.h"
#include "Engine/DataTable.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UStoryComponent::UStoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UStoryComponent::PlayDialogue(const TArray<FName>& LineIDs, bool bLockInput, FSimpleDelegate OnFinished)
{
	if (bIsPlaying || LineIDs.Num() == 0 || !DialogueTable || !DialogueWidgetClass)
		return false;

	PendingLines = LineIDs;
	CurrentLineIndex = 0;
	bIsPlaying = true;
	bInputLocked = bLockInput;
	FinishedCallback = OnFinished;

	if (!IsValid(DialogueWidget))
	{
		DialogueWidget = CreateWidget<UUI_Dialogue>(GetWorld(), DialogueWidgetClass);
		if (!IsValid(DialogueWidget))
		{
			bIsPlaying = false;
			return false;
		}
		// 위젯의 [계속] 버튼 → 컴포넌트로 되돌아오는 유일한 경로
		DialogueWidget->OnAdvanceRequested.BindUObject(this, &UStoryComponent::AdvanceLine);
	}

	DialogueWidget->AddToViewport(10);
	if (bInputLocked)
	{
		SetDialogueInputMode(true);
	}

	ShowCurrentLine();
	return true;
}

bool UStoryComponent::PlayDialogueOnce(FName BeatID, const TArray<FName>& LineIDs, bool bLockInput, FSimpleDelegate OnFinished)
{
	if (!BeatID.IsNone())
	{
		if (UStoryStateSubsystem* SS = UGameplayStatics::GetGameInstance(this)->GetSubsystem<UStoryStateSubsystem>())
		{
			if (SS->HasSeen(BeatID))
				return false;
		}
	}

	if (!PlayDialogue(LineIDs, bLockInput, OnFinished))
		return false;

	if (!BeatID.IsNone())
	{
		if (UStoryStateSubsystem* SS = UGameplayStatics::GetGameInstance(this)->GetSubsystem<UStoryStateSubsystem>())
		{
			SS->MarkSeen(BeatID);
		}
	}

	return true;
}

void UStoryComponent::ShowCurrentLine()
{
	GetWorld()->GetTimerManager().ClearTimer(AutoAdvanceTimerHandle);

	const FName LineID = PendingLines[CurrentLineIndex];
	static const FString Ctx(TEXT("StoryComponent"));
	const FDialogueLine* Row = DialogueTable->FindRow<FDialogueLine>(LineID, Ctx, false);
	if (!Row)
	{
		// 대사 ID 오타는 조용히 넘기지 않는다 — 대본과 데이터가 어긋난 것이므로 로그로 드러낸다
		LOG(TEXT("Dialogue row not found: %s"), *LineID.ToString());
		AdvanceLine();
		return;
	}

	DialogueWidget->SetLine(*Row);

	if (Row->AutoAdvanceTime > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(AutoAdvanceTimerHandle, this,
			&UStoryComponent::AdvanceLine, Row->AutoAdvanceTime, false);
	}
}

void UStoryComponent::AdvanceLine()
{
	if (!bIsPlaying)
		return;

	++CurrentLineIndex;
	if (PendingLines.IsValidIndex(CurrentLineIndex))
	{
		ShowCurrentLine();
		return;
	}
	FinishDialogue();
}

void UStoryComponent::StopDialogue()
{
	if (!bIsPlaying)
		return;

	GetWorld()->GetTimerManager().ClearTimer(AutoAdvanceTimerHandle);
	if (IsValid(DialogueWidget))
	{
		DialogueWidget->RemoveFromParent();
	}
	if (bInputLocked)
	{
		SetDialogueInputMode(false);
	}

	bIsPlaying = false;
	PendingLines.Reset();
	FinishedCallback.Unbind();
}

void UStoryComponent::FinishDialogue()
{
	GetWorld()->GetTimerManager().ClearTimer(AutoAdvanceTimerHandle);
	if (IsValid(DialogueWidget))
	{
		DialogueWidget->RemoveFromParent();
	}
	if (bInputLocked)
	{
		SetDialogueInputMode(false);
	}

	bIsPlaying = false;
	PendingLines.Reset();

	// 콜백 안에서 다시 PlayDialogue를 호출할 수 있으므로(완료 대사 → 다음 인트로) 먼저 비우고 실행한다
	FSimpleDelegate Callback = FinishedCallback;
	FinishedCallback.Unbind();
	Callback.ExecuteIfBound();
}

void UStoryComponent::SetDialogueInputMode(bool bEnable)
{
	APy_Grey* Player = Cast<APy_Grey>(GetOwner());
	if (!Player)
		return;

	Player->SetGameplayInputLocked(bEnable);

	APlayerController* PC = Cast<APlayerController>(Player->GetController());
	if (!PC)
		return;

	if (bEnable)
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeGameAndUI());
	}
	else
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void UStoryComponent::ShowChapterTitle(const FText& ChapterLabel, const FText& ChapterTitle)
{
	if (!ChapterTitleWidgetClass)
		return;

	if (!IsValid(ChapterTitleWidget))
	{
		ChapterTitleWidget = CreateWidget<UUI_ChapterTitle>(GetWorld(), ChapterTitleWidgetClass);
	}
	if (!IsValid(ChapterTitleWidget))
		return;

	ChapterTitleWidget->SetChapter(ChapterLabel, ChapterTitle);
	ChapterTitleWidget->AddToViewport(20);

	GetWorld()->GetTimerManager().SetTimer(ChapterTitleTimerHandle, this,
		&UStoryComponent::RemoveChapterTitle, ChapterTitleDuration, false);
}

void UStoryComponent::RemoveChapterTitle()
{
	if (IsValid(ChapterTitleWidget))
	{
		ChapterTitleWidget->RemoveFromParent();
	}
}

void UStoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(AutoAdvanceTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(ChapterTitleTimerHandle);

	if (IsValid(DialogueWidget))
	{
		DialogueWidget->RemoveFromParent();
		DialogueWidget = nullptr;
	}
	if (IsValid(ChapterTitleWidget))
	{
		ChapterTitleWidget->RemoveFromParent();
		ChapterTitleWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}
