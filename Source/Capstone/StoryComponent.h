#pragma once

#include "GameInfo.h"
#include "Components/ActorComponent.h"
#include "StoryComponent.generated.h"

// 대사 시퀀스 재생기 (11단계). APy_Grey가 소유하며, 퀘스트 NPC/보스/스토리 트리거가 이를 통해 대사를 재생한다.
// 대사 재생 지점은 여러 곳(NPC, 보스, 트리거)에 흩어져 있지만, 대사창 위젯 인스턴스와 입력 잠금 상태는
// 항상 이 컴포넌트 하나에서만 관리한다
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CAPSTONE_API UStoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStoryComponent();

	// LineIDs를 순서대로 재생한다. 이미 재생 중이면 아무것도 하지 않고 false를 반환한다.
	// bLockInput=true면 공격/스킬 입력을 막고 마우스 커서를 켠다(대화용). 전투 중 대사는 false로 넘긴다.
	// OnFinished는 시퀀스가 끝난 뒤 1회 호출되고 즉시 비워진다.
	// FSimpleDelegate는 UHT가 블루프린트 파라미터로 반영할 수 없으므로 UFUNCTION으로 노출하지 않는다 (C++ 전용 호출)
	bool PlayDialogue(const TArray<FName>& LineIDs, bool bLockInput = true, FSimpleDelegate OnFinished = FSimpleDelegate());

	// BeatID를 아직 본 적 없을 때만 재생하고, 재생을 시작하면 즉시 Seen으로 기록한다
	bool PlayDialogueOnce(FName BeatID, const TArray<FName>& LineIDs, bool bLockInput = true, FSimpleDelegate OnFinished = FSimpleDelegate());

	// 재생 중인 시퀀스를 즉시 끝낸다 (완료 콜백은 호출하지 않는다)
	UFUNCTION(BlueprintCallable, Category = "Story")
	void StopDialogue();

	// 다음 줄로. E키(APy_Grey) / 대사창 버튼 클릭에서 호출
	UFUNCTION(BlueprintCallable, Category = "Story")
	void AdvanceLine();

	UFUNCTION(BlueprintPure, Category = "Story")
	bool IsPlaying() const { return bIsPlaying; }

	// 챕터 타이틀 카드를 ChapterTitleDuration초 동안 표시한다 (대사 재생과 독립적으로 동작)
	UFUNCTION(BlueprintCallable, Category = "Story")
	void ShowChapterTitle(const FText& ChapterLabel, const FText& ChapterTitle);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "Story")
	class UDataTable* DialogueTable;

	UPROPERTY(EditDefaultsOnly, Category = "Story")
	TSubclassOf<class UUI_Dialogue> DialogueWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Story")
	TSubclassOf<class UUI_ChapterTitle> ChapterTitleWidgetClass;

	UPROPERTY(EditAnywhere, Category = "Story")
	float ChapterTitleDuration = 2.5f;

private:
	void ShowCurrentLine();
	void FinishDialogue();
	void SetDialogueInputMode(bool bEnable);
	void RemoveChapterTitle();

	UPROPERTY()
	class UUI_Dialogue* DialogueWidget = nullptr;

	UPROPERTY()
	class UUI_ChapterTitle* ChapterTitleWidget = nullptr;

	TArray<FName> PendingLines;
	int32 CurrentLineIndex = 0;
	bool bIsPlaying = false;
	bool bInputLocked = false;
	FSimpleDelegate FinishedCallback;
	FTimerHandle AutoAdvanceTimerHandle;
	FTimerHandle ChapterTitleTimerHandle;
};
