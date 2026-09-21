#include "StoryComponent.h"
#include "DialogueData.h"
#include "UI_Dialogue.h"
#include "UI_ChapterTitle.h"
#include "StoryStateSubsystem.h"
#include "CapstoneSFX.h"
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
	CapstoneSFX::Play2D(this, ESFX::UI_DialogueOpen);

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
		CapstoneSFX::Play2D(this, ESFX::UI_DialogueNext);
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

	bIsPlaying = false;
	PendingLines.Reset();

	// 입력 모드는 콜백보다 먼저 원복한다. 콜백(예: 퀘스트 수락창 열기)이 자기 나름의 입력 모드/마우스
	// 커서 상태를 설정할 수 있는데, 이걸 콜백 실행 뒤에 원복해버리면 콜백이 방금 켠 마우스 커서를
	// 이 함수가 다시 꺼버리는 문제가 생긴다(퀘스트 수락/거절창에서 마우스 커서가 사라지는 버그).
	// 대사가 콜백 안에서 곧바로 이어지는 경우(아래)에는 PlayDialogue가 SetDialogueInputMode(true)를
	// 다시 걸어주므로 여기서 먼저 꺼도 문제없다.
	if (bInputLocked)
	{
		SetDialogueInputMode(false);
	}

	// 콜백 안에서 다시 PlayDialogue를 호출할 수 있으므로(완료 대사 → 다음 인트로) 먼저 비우고 실행한다.
	// 이때 위젯을 미리 RemoveFromParent()로 내렸다가 콜백이 같은 프레임에 PlayDialogue로 다시
	// AddToViewport()하면, UMG가 그 위젯의 NativeConstruct를 재실행해서 버튼 델리게이트가 중복
	// 바인딩되고(엔진 ensure: InvocationList[...] != InDelegate) 화면이 마지막 줄에서 멈춘 것처럼
	// 보이는 버그가 있었다. 콜백을 먼저 실행해서 새 대사가 바로 시작됐는지(bIsPlaying) 확인한 뒤에만
	// 위젯을 정리한다 — 새 대사가 이어지면 위젯은 그대로 두고 화면 갱신은 그쪽에 맡긴다.
	FSimpleDelegate Callback = FinishedCallback;
	FinishedCallback.Unbind();
	Callback.ExecuteIfBound();

	if (bIsPlaying)
		return;

	if (IsValid(DialogueWidget))
	{
		DialogueWidget->RemoveFromParent();
	}
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
	CapstoneSFX::Play2D(this, ESFX::UI_ChapterTitle);

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
