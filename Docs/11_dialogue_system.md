# 11단계: 대사 · 연출 시스템 구현 (C++)

## 목표

10단계 대본을 실제로 화면에 띄운다. **대사 DataTable → 순차 재생기 → 대사창 위젯**을 만들고, 기존 퀘스트 NPC / 대장장이 / 보스 / 엔딩 흐름에 재생 지점을 끼워 넣는다. 스토리 1회성 이벤트(프롤로그, 보스 조우)는 레벨을 넘나들어도 다시 재생되지 않아야 한다.

## 선행 단계

9단계(설계), 10단계(대사 ID 확정). 코드상으로는 5단계(퀘스트 체인)와 8단계(보스/엔딩)가 필요하다.

## 읽어야 할 파일

`GameInfo.h`(`FQuestData`), `NPC_Quest.h/.cpp`, `NPC_Enhance.h/.cpp`, `Py_Grey.h/.cpp`(`HandleBossDefeated`/`ShowEnding`/`OnInteractKeyPressed`), `Mon_Narbash.h/.cpp`(`EnterPhase2`), `PortalPoint.cpp`(게이트 문구), `UI_Quest.h`(위젯 관례 참고), `ItemData.h`(DataTable 행 구조체 관례 참고).

## 신규 / 수정 파일

| 파일 | 구분 | 내용 |
|---|---|---|
| `DialogueData.h` | 신규 | `FDialogueLine : FTableRowBase` |
| `StoryStateSubsystem.h/.cpp` | 신규 | 1회성 이벤트 기록 (`UGameInstanceSubsystem`) |
| `StoryComponent.h/.cpp` | 신규 | 대사 시퀀스 재생기 (플레이어 컴포넌트) |
| `UI_Dialogue.h/.cpp` | 신규 | 하단 대사창 위젯 |
| `UI_ChapterTitle.h/.cpp` | 신규 | 챕터 타이틀 카드 위젯 |
| `StoryTrigger.h/.cpp` | 신규 | 레벨 배치용 대사 트리거 액터 |
| `GameInfo.h` | 수정 | `FQuestData`에 스토리 필드 4개 추가 |
| `NPC_Quest.h/.cpp` | 수정 | 인트로/진행/완료 대사 → 수락창 → 챕터 카드 흐름 |
| `NPC_Enhance.h/.cpp` | 수정 | 고든 첫 대면 대사 1회 |
| `Mon_Narbash.h/.cpp` | 수정 | 페이즈2 전환 대사 |
| `Py_Grey.h/.cpp` | 수정 | `StoryComponent` 소유, 보스 사망 대사 → 에필로그 → 엔딩, E키 대사 넘기기 |
| `PortalPoint.cpp` | 수정 | 게이트 거부 문구 교체 (1줄) |

> 한글 주석이 들어가는 신규 `.h`/`.cpp`는 **UTF-8 BOM**으로 저장한다 (`CLAUDE.md` 규칙).

## 설계

### 1. `DialogueData.h` — 대사 행 구조체

`ItemData.h`와 같은 관례(독립 헤더 + `FTableRowBase`).

```cpp
#pragma once

#include "Engine/DataTable.h"
#include "DialogueData.generated.h"

USTRUCT(BlueprintType)
struct FDialogueLine : public FTableRowBase
{
    GENERATED_BODY()

    // 비어 있으면 나레이션으로 간주하지 않고 이름표만 숨긴다
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    FText SpeakerName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (MultiLine = true))
    FText LineText;

    // true면 초상화/이름표를 숨기고 전체 화면 딤을 켠다 (프롤로그/에필로그)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    bool bIsNarration = false;

    // 0이면 입력 대기. 0보다 크면 그 초 후 자동으로 다음 줄 (전투 중 대사)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    float AutoAdvanceTime = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    TSoftObjectPtr<UTexture2D> PortraitIcon;
};
```

### 2. `UStoryStateSubsystem` — 1회성 이벤트 기록

레벨을 이동해도(`OpenLevel`) 살아남아야 하므로 액터가 아니라 **게임 인스턴스 서브시스템**에 둔다. 별도 등록 절차 없이 엔진이 자동 생성한다.

```cpp
// StoryStateSubsystem.h
#pragma once

#include "GameInfo.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StoryStateSubsystem.generated.h"

// 프롤로그/보스 조우처럼 "한 번만" 재생돼야 하는 연출의 기록. 게임 인스턴스 수명 동안 유지된다
// (세이브 기능은 범위 밖 — 게임을 재실행하면 초기화된다. 9단계 "만들지 않는 것" 참고)
UCLASS()
class CAPSTONE_API UStoryStateSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Story")
    bool HasSeen(FName BeatID) const { return !BeatID.IsNone() && SeenBeats.Contains(BeatID); }

    UFUNCTION(BlueprintCallable, Category = "Story")
    void MarkSeen(FName BeatID) { if (!BeatID.IsNone()) SeenBeats.Add(BeatID); }

    UFUNCTION(BlueprintCallable, Category = "Story")
    void SetChapter(int32 InChapter) { CurrentChapter = InChapter; }

    UFUNCTION(BlueprintPure, Category = "Story")
    int32 GetChapter() const { return CurrentChapter; }

private:
    TSet<FName> SeenBeats;
    int32 CurrentChapter = 0;
};
```

접근은 `GetWorld()->GetGameInstance()->GetSubsystem<UStoryStateSubsystem>()`.

### 3. `UStoryComponent` — 대사 시퀀스 재생기

플레이어(`APy_Grey`)가 소유한다. 대사 재생 지점이 NPC·보스·트리거로 흩어져 있어도 **위젯 인스턴스와 입력 상태는 항상 한 곳에서만 관리**되도록 하기 위함이다.

```cpp
// StoryComponent.h (요지)
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CAPSTONE_API UStoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStoryComponent();

    // LineIDs를 순서대로 재생. 이미 재생 중이면 아무것도 하지 않고 false.
    // bLockInput=true면 공격/스킬 입력을 막고 커서를 켠다(대화용). 전투 중 대사는 false로.
    // OnFinished는 시퀀스가 끝난 뒤 1회 호출되고 즉시 비워진다.
    bool PlayDialogue(const TArray<FName>& LineIDs, bool bLockInput = true,
                      FSimpleDelegate OnFinished = FSimpleDelegate());

    // BeatID를 아직 본 적 없을 때만 재생하고, 재생을 시작하면 즉시 Seen으로 기록한다
    bool PlayDialogueOnce(FName BeatID, const TArray<FName>& LineIDs, bool bLockInput = true,
                          FSimpleDelegate OnFinished = FSimpleDelegate());

    // 재생 중인 시퀀스를 즉시 끝낸다(완료 콜백은 호출하지 않는다)
    void StopDialogue();

    // 다음 줄로. E키(APy_Grey) / 대사창 버튼 클릭에서 호출
    void AdvanceLine();

    bool IsPlaying() const { return bIsPlaying; }

    // 챕터 타이틀 카드를 DisplayDuration 동안 표시 (대사와 독립적으로 동작)
    void ShowChapterTitle(const FText& ChapterLabel, const FText& ChapterTitle);

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditDefaultsOnly, Category = "Story")
    class UDataTable* DialogueTable;               // DT_Dialogue

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

    UPROPERTY() class UUI_Dialogue* DialogueWidget = nullptr;
    UPROPERTY() class UUI_ChapterTitle* ChapterTitleWidget = nullptr;

    TArray<FName> PendingLines;
    int32 CurrentLineIndex = 0;
    bool bIsPlaying = false;
    bool bInputLocked = false;
    FSimpleDelegate FinishedCallback;
    FTimerHandle AutoAdvanceTimerHandle;
    FTimerHandle ChapterTitleTimerHandle;
};
```

핵심 구현 (`StoryComponent.cpp`):

```cpp
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

    DialogueWidget->AddToViewport(10);   // HUD(기본 0)보다 위, 챕터 카드(20)보다 아래
    if (bInputLocked) SetDialogueInputMode(true);

    ShowCurrentLine();
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
    if (!bIsPlaying) return;

    ++CurrentLineIndex;
    if (PendingLines.IsValidIndex(CurrentLineIndex))
    {
        ShowCurrentLine();
        return;
    }
    FinishDialogue();
}

void UStoryComponent::FinishDialogue()
{
    GetWorld()->GetTimerManager().ClearTimer(AutoAdvanceTimerHandle);
    if (IsValid(DialogueWidget)) DialogueWidget->RemoveFromParent();
    if (bInputLocked) SetDialogueInputMode(false);

    bIsPlaying = false;
    PendingLines.Reset();

    // 콜백 안에서 다시 PlayDialogue를 호출할 수 있으므로(완료 대사 → 다음 인트로) 먼저 비우고 실행한다
    FSimpleDelegate Callback = FinishedCallback;
    FinishedCallback.Unbind();
    Callback.ExecuteIfBound();
}
```

`SetDialogueInputMode(true)`는 기존 NPC 위젯들과 같은 방식이다: 소유자를 `APy_Grey`로 캐스팅해 `SetGameplayInputLocked(true)`, 컨트롤러에 `bShowMouseCursor = true` + `SetInputMode(FInputModeGameAndUI())`. `false`면 그 반대. **이동은 막지 않는다** — 기존 상점/강화 UI와 동일한 정책이며, 이동을 얼리려면 별도 연출 작업이 필요해 범위 밖이다.

`EndPlay`에서 타이머 2개를 정리하고 위젯을 `RemoveFromParent`한다.

### 4. `UUI_Dialogue` — 하단 대사창

```cpp
UCLASS()
class CAPSTONE_API UUI_Dialogue : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetLine(const struct FDialogueLine& Line);

    // StoryComponent가 바인딩한다 (버튼 클릭 → 다음 줄)
    FSimpleDelegate OnAdvanceRequested;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(meta = (BindWidgetOptional)) class UTextBlock* SpeakerText;
    UPROPERTY(meta = (BindWidgetOptional)) class UTextBlock* LineText;
    UPROPERTY(meta = (BindWidgetOptional)) class UImage*     PortraitImage;
    UPROPERTY(meta = (BindWidgetOptional)) class UImage*     NarrationBackground;
    UPROPERTY(meta = (BindWidgetOptional)) class UTextBlock* ContinueHintText;
    UPROPERTY(meta = (BindWidgetOptional)) class UButton*    NextButton;

    // 0이면 타자기 효과 없이 즉시 전체 표시
    UPROPERTY(EditAnywhere, Category = "Dialogue")
    float CharsPerSecond = 40.f;

    UFUNCTION()
    void OnNextClicked();

private:
    FString FullLine;       // 현재 줄 전체
    float RevealedChars = 0.f;
    bool bRevealing = false;
};
```

- `SetLine`: 화자 이름이 비었거나 `bIsNarration`이면 `SpeakerText`/`PortraitImage`를 `Collapsed`, `NarrationBackground`를 `Visible`로. 아니면 그 반대. `PortraitIcon`이 비어 있으면 `PortraitImage`는 항상 숨긴다(초상화 텍스처가 없어도 정상 동작).
- 타자기 효과: `NativeTick`에서 `RevealedChars += CharsPerSecond * InDeltaTime`, `LineText`에 `FullLine.Left(...)`를 넣는다. 다 나오면 `bRevealing = false` + `ContinueHintText` 표시.
- `OnNextClicked`: **아직 글자가 다 안 나왔으면 먼저 전체를 즉시 표시**하고, 이미 다 나왔으면 `OnAdvanceRequested.ExecuteIfBound()`. (대사 스킵 관례)

### 5. `UUI_ChapterTitle` — 챕터 타이틀 카드

```cpp
UCLASS()
class CAPSTONE_API UUI_ChapterTitle : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Story")
    void SetChapter(const FText& InLabel, const FText& InTitle);

protected:
    virtual void NativeConstruct() override;   // FadeAnim이 있으면 재생

    UPROPERTY(meta = (BindWidgetOptional)) class UTextBlock* ChapterLabelText;
    UPROPERTY(meta = (BindWidgetOptional)) class UTextBlock* ChapterTitleText;

    // UMG 애니메이션. 없으면 그냥 즉시 표시되고 컴포넌트 타이머가 제거한다
    UPROPERTY(meta = (BindWidgetAnimOptional), Transient)
    class UWidgetAnimation* FadeAnim;
};
```

`UStoryComponent::ShowChapterTitle`은 위젯을 만들어 `AddToViewport(20)` 하고 `ChapterTitleDuration` 타이머 후 `RemoveChapterTitle()`을 호출한다. 페이드 인/아웃은 UMG 애니메이션(사용자 제작)에 맡기고 코드는 재생만 한다.

### 6. `AStoryTrigger` — 레벨 배치용 대사 트리거

```cpp
UENUM(BlueprintType)
enum class EStoryTriggerMode : uint8
{
    Overlap,      // 플레이어가 볼륨에 들어오면
    LevelStart    // 레벨 시작 후 StartDelay초 뒤 (프롤로그용)
};

UCLASS()
class CAPSTONE_API AStoryTrigger : public AActor
{
    GENERATED_BODY()

public:
    AStoryTrigger();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Story")
    class UBoxComponent* TriggerBox;

    UPROPERTY(EditAnywhere, Category = "Story")
    EStoryTriggerMode TriggerMode = EStoryTriggerMode::Overlap;

    UPROPERTY(EditAnywhere, Category = "Story")
    TArray<FName> DialogueLines;

    // 비워두면 들어올 때마다 재생. 값을 넣으면 게임 인스턴스 수명 동안 1회만 재생
    UPROPERTY(EditAnywhere, Category = "Story")
    FName BeatID;

    UPROPERTY(EditAnywhere, Category = "Story")
    bool bLockInput = true;

    UPROPERTY(EditAnywhere, Category = "Story", meta = (EditCondition = "TriggerMode == EStoryTriggerMode::LevelStart"))
    float StartDelay = 0.5f;

    UPROPERTY(EditAnywhere, Category = "Story")
    bool bDestroyAfterPlay = true;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    void TryPlay(AActor* PlayerActor);

private:
    FTimerHandle StartTimerHandle;
};
```

- `BeginPlay`: 오버랩 델리게이트 바인딩은 **`BeginPlay`에서 1회만** (`APortalPoint`에서 `Tick` 바인딩으로 겪었던 중복 등록 버그 재발 방지). `LevelStart` 모드면 `StartDelay` 타이머로 `TryPlay(UGameplayStatics::GetPlayerPawn(this, 0))`.
- `TryPlay`: 플레이어에서 `UStoryComponent`를 얻어 `BeatID`가 있으면 `PlayDialogueOnce`, 없으면 `PlayDialogue`. 재생에 성공하고 `bDestroyAfterPlay`면 `Destroy()`.

### 7. `FQuestData` 스토리 필드 추가 (`GameInfo.h`)

퀘스트 정의와 대사가 같은 자리에서 편집돼야 에디터에서 실수가 없다. 기존 필드 아래에 추가:

```cpp
    // 9~11단계 스토리. 전부 선택 사항 — 비워두면 대사 없이 기존 동작 그대로다
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Story")
    FText ChapterLabel;                         // "제 2장" (챕터 제목은 QuestTitle을 그대로 쓴다)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Story")
    TArray<FName> IntroDialogueLines;           // 수락창 열기 전

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Story")
    TArray<FName> InProgressDialogueLines;      // 진행 중 다시 말 걸었을 때

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Story")
    TArray<FName> CompleteDialogueLines;        // 완료 후 다시 말 걸었을 때
```

### 8. `ANPC_Quest` — 대화 흐름 삽입

기존 `Interact_Implementation`을 **3조각으로 쪼갠다**. 대사가 비동기(플레이어 입력 대기)로 끝나므로 "대사 끝 → 다음 단계"를 콜백으로 이어야 하기 때문이다.

```cpp
// NPC_Quest.h 추가
void BeginNextQuestIntro();     // 인트로 대사 → OpenQuestWidget
void OpenQuestWidget();         // 기존 수락창 생성 블록 (그대로 이동)
```

```cpp
void ANPC_Quest::Interact_Implementation(AActor* Interactor)
{
    if (IsValid(QuestWidget)) return;
    if (!QuestChain.IsValidIndex(CurrentChainIndex)) return;

    APy_Grey* Player = Cast<APy_Grey>(Interactor);
    if (!Player || !Player->QuestComponent) return;

    UStoryComponent* Story = Player->StoryComponent;
    if (Story && Story->IsPlaying()) return;    // 대사 재생 중 재진입 차단

    UQuestComponent* QC = Player->QuestComponent;
    const FName WantQuestID = QuestChain[CurrentChainIndex].QuestID;

    if (QC->HasActiveQuest())
    {
        const FQuestData& Active = QC->GetActiveQuest();
        if (Active.QuestID != WantQuestID) return;

        if (!Active.bIsCompleted)
        {
            // 진행 중 — 대사만 한 줄, 수락창은 열지 않는다 (기존 동작 유지)
            const TArray<FName>& ProgLines = QuestChain[CurrentChainIndex].InProgressDialogueLines;
            if (Story && ProgLines.Num() > 0) Story->PlayDialogue(ProgLines);
            return;
        }

        // 방금 완료됨 → 완료 대사 후 체인 다음 칸으로
        CurrentInteractor = Interactor;
        const TArray<FName> DoneLines = QuestChain[CurrentChainIndex].CompleteDialogueLines;
        ++CurrentChainIndex;

        if (Story && DoneLines.Num() > 0)
        {
            Story->PlayDialogue(DoneLines, true,
                FSimpleDelegate::CreateUObject(this, &ANPC_Quest::BeginNextQuestIntro));
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
        CurrentInteractor = nullptr;    // 체인 끝 — 더 줄 퀘스트 없음
        return;
    }

    APy_Grey* Player = Cast<APy_Grey>(CurrentInteractor);
    UStoryComponent* Story = Player ? Player->StoryComponent : nullptr;
    const TArray<FName>& IntroLines = QuestChain[CurrentChainIndex].IntroDialogueLines;

    if (Story && IntroLines.Num() > 0)
    {
        Story->PlayDialogue(IntroLines, true,
            FSimpleDelegate::CreateUObject(this, &ANPC_Quest::OpenQuestWidget));
        return;
    }
    OpenQuestWidget();
}
```

`OpenQuestWidget()`은 기존 `Super::Interact_Implementation(...)` 이후의 위젯 생성 블록을 그대로 옮긴 것이다 (`CurrentInteractor`를 쓰도록 인자만 제거).

**챕터 카드는 수락 직후에 띄운다** — 대사 → 수락창 → 카드 → 플레이 순서라 콜백 체인이 늘어나지 않는다:

```cpp
void ANPC_Quest::HandleQuestAccepted()
{
    APy_Grey* Player = Cast<APy_Grey>(CurrentInteractor);
    if (Player && Player->QuestComponent && QuestChain.IsValidIndex(CurrentChainIndex))
    {
        const FQuestData& Q = QuestChain[CurrentChainIndex];
        Player->QuestComponent->AcceptQuest(Q);

        if (Player->StoryComponent && !Q.ChapterLabel.IsEmpty())
        {
            Player->StoryComponent->ShowChapterTitle(Q.ChapterLabel, Q.QuestTitle);
            if (UStoryStateSubsystem* SS = GetGameInstance()->GetSubsystem<UStoryStateSubsystem>())
                SS->SetChapter(CurrentChainIndex + 1);
        }
    }

    RestoreInputMode();
    QuestWidget = nullptr;
    CurrentInteractor = nullptr;
}
```

### 9. `ANPC_Enhance` — 고든 첫 대면 대사

기존 위젯 생성 블록을 `OpenEnhanceWidget()`으로 추출하고, 앞에 1회성 대사를 끼운다.

```cpp
// NPC_Enhance.h 추가
UPROPERTY(EditAnywhere, Category = "Story")
TArray<FName> FirstMeetDialogueLines;

UPROPERTY(EditAnywhere, Category = "Story")
FName FirstMeetBeatID = TEXT("Beat_GodonFirstMeet");

void OpenEnhanceWidget();
```

```cpp
void ANPC_Enhance::Interact_Implementation(AActor* Interactor)
{
    if (IsValid(EnhanceWidget)) return;

    APy_Grey* Player = Cast<APy_Grey>(Interactor);
    if (!Player || !Player->InventoryComponent) return;

    UStoryComponent* Story = Player->StoryComponent;
    if (Story && Story->IsPlaying())
        return; // 다른 대사 재생 중 재진입 차단

    CurrentInteractor = Interactor;

    if (Story && FirstMeetDialogueLines.Num() > 0)
    {
        // 처음 만났을 때만 대사 → 끝나면 강화창. 두 번째부터는 곧장 강화창
        if (Story->PlayDialogueOnce(FirstMeetBeatID, FirstMeetDialogueLines, true,
                FSimpleDelegate::CreateUObject(this, &ANPC_Enhance::OpenEnhanceWidget)))
            return;
    }

    OpenEnhanceWidget();
}
```

`OpenEnhanceWidget()`은 기존 코드 그대로(`Super::Interact_Implementation(CurrentInteractor)` 포함).

### 10. `AMon_Narbash` — 페이즈2 대사

```cpp
// Mon_Narbash.h — Boss 카테고리 아래
UPROPERTY(EditAnywhere, Category = "Boss|Story")
TArray<FName> Phase2DialogueLines;
```

`EnterPhase2()` 끝부분에 추가. **전투 중이므로 입력을 막지 않는다**(`bLockInput = false`) — 대사는 `AutoAdvanceTime`으로 알아서 넘어간다(10단계에서 2.5초로 지정).

```cpp
    if (APy_Grey* Player = Cast<APy_Grey>(UGameplayStatics::GetPlayerPawn(this, 0)))
    {
        if (Player->StoryComponent && Phase2DialogueLines.Num() > 0)
            Player->StoryComponent->PlayDialogue(Phase2DialogueLines, false);
    }
```

보스 **조우** 대사는 코드가 필요 없다 — 아레나 입구에 `AStoryTrigger`(`BeatID = Beat_BossMeet`)를 배치하면 된다.

### 11. `APy_Grey` — 컴포넌트 소유 · 보스 사망 대사 · 에필로그

```cpp
// Py_Grey.h
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
class UStoryComponent* StoryComponent;

UPROPERTY(EditAnywhere, Category = "Story")
TArray<FName> BossDeathDialogueLines;    // DL_BOSS_DEAD_01

UPROPERTY(EditAnywhere, Category = "Story")
TArray<FName> EpilogueDialogueLines;     // DL_EPI_01~03

void PlayEpilogue();
```

생성자에 `StoryComponent = CreateDefaultSubobject<UStoryComponent>(TEXT("StoryComponent"));`.

E키로 대사를 넘길 수 있게 `OnInteractKeyPressed` 맨 앞에 삽입 (마우스 클릭 없이도 진행 가능하게):

```cpp
void APy_Grey::OnInteractKeyPressed()
{
    if (StoryComponent && StoryComponent->IsPlaying())
    {
        StoryComponent->AdvanceLine();
        return;                 // 대사 중에는 아이템 줍기/NPC 상호작용을 하지 않는다
    }
    ... 기존 코드 ...
}
```

엔딩 흐름 (기존 `HandleBossDefeated` → `ShowEnding` 사이에 에필로그 한 단계 삽입):

```cpp
void APy_Grey::HandleBossDefeated(FName MonsterID)
{
    // 보스 유언은 쓰러지는 연출 위에 겹쳐 나와야 하므로 입력을 막지 않는다
    if (StoryComponent && BossDeathDialogueLines.Num() > 0)
        StoryComponent->PlayDialogue(BossDeathDialogueLines, false);

    GetWorldTimerManager().SetTimer(EndingTimerHandle, this, &APy_Grey::PlayEpilogue, EndingDelay, false);
}

void APy_Grey::PlayEpilogue()
{
    if (StoryComponent)
    {
        StoryComponent->StopDialogue();     // 유언이 아직 떠 있으면 정리하고 시작
        if (EpilogueDialogueLines.Num() > 0 &&
            StoryComponent->PlayDialogue(EpilogueDialogueLines, true,
                FSimpleDelegate::CreateUObject(this, &APy_Grey::ShowEnding)))
            return;
    }
    ShowEnding();       // 대사 데이터가 없으면 기존 8단계 동작 그대로
}
```

`ShowEnding()`은 변경하지 않는다. 엔딩 화면의 마지막 한 줄(10단계 "엔딩 화면 고정 문구")은 `WBP_Ending`에 **정적 텍스트**로 넣으므로 코드가 관여하지 않는다.

### 12. `APortalPoint` — 문구 1줄 교체

```cpp
PrintViewport(2.f, FColor::Red, TEXT("아직은 아니다. 하란에게 먼저 얘기하자."));
```

## 에디터 작업 (사용자)

1. **빌드** 후 `DT_Dialogue` 임포트 (10단계 CSV, 행 구조체 `FDialogueLine`).
2. **위젯 3종 제작** — `WBP_Dialogue`, `WBP_ChapterTitle`, `WBP_Ending` 수정. 상세 명세는 **12단계 문서** 참조.
3. **BP_PyGrey** → `StoryComponent` 선택 후: `DialogueTable = DT_Dialogue`, `DialogueWidgetClass = WBP_Dialogue`, `ChapterTitleWidgetClass = WBP_ChapterTitle`. 액터 본체의 `Story` 카테고리에 `BossDeathDialogueLines = [DL_BOSS_DEAD_01]`, `EpilogueDialogueLines = [DL_EPI_01, DL_EPI_02, DL_EPI_03]`.
4. **하란(퀘스트 NPC)** `QuestChain` 4항목에 10단계 대본대로 입력:
   - `[0]` `ChapterLabel="제 1장"`, Intro=`DL_C1_INTRO_01`~`05`, InProgress=`DL_C1_PROG_01`, Complete=`DL_C1_DONE_01,02`
   - `[1]` `ChapterLabel="제 2장"`, Intro=`DL_C2_INTRO_01`~`04`, InProgress=`DL_C2_PROG_01`, Complete=`DL_C2_DONE_01,02`
   - `[2]` `ChapterLabel="제 3장"`, Intro=`DL_C3_INTRO_01`~`03`, InProgress=`DL_C3_PROG_01`, Complete=`DL_C3_DONE_01`
   - `[3]` `ChapterLabel="제 4장"`, Intro=`DL_C4_INTRO_01`~`04`, InProgress=`DL_C4_PROG_01`, Complete=(비움 — 보스 처치 후엔 에필로그로 간다)
5. **고든(강화 NPC)**: `FirstMeetDialogueLines = [DL_C3_GODON_01, 02, 03]`.
6. **나바쉬 BP**: `Phase2DialogueLines = [DL_BOSS_P2_01]`.
7. **`AStoryTrigger` 배치 2개**
   - `Lv_Village`: `TriggerMode=LevelStart`, `StartDelay=0.5`, `BeatID=Beat_Prologue`, `DialogueLines=[DL_P_01~04]`, `bLockInput=true`.
   - `Lv_Boss` 아레나 입구: `TriggerMode=Overlap`, `BeatID=Beat_BossMeet`, `DialogueLines=[DL_BOSS_MEET_01~04]`, 박스를 문 폭에 맞게 키운다.
8. (선택) 초상화 텍스처 4종을 `DT_Dialogue` 행의 `PortraitIcon`에 지정.

## 완료 기준 (PIE 테스트)

- [ ] 마을에 처음 들어가면 프롤로그 4줄이 뜨고, 포탈로 나갔다 돌아와도 **다시 뜨지 않는다**.
- [ ] 하란에게 말 걸면 인트로 대사 → 수락창 → [수락] → 챕터 카드 순으로 진행된다.
- [ ] 퀘스트 진행 중 다시 말 걸면 진행 중 대사 1줄만 뜨고 수락창은 열리지 않는다.
- [ ] 퀘스트 완료 후 말 걸면 완료 대사 → 곧바로 다음 장 인트로 대사 → 수락창으로 이어진다.
- [ ] 고든에게 처음 말 걸면 대사 3줄 후 강화창이 열리고, 두 번째부터는 곧장 강화창이 열린다.
- [ ] 대사 중 E키와 마우스 클릭 둘 다 다음 줄로 넘긴다. 글자가 나오는 중에 누르면 전체가 즉시 표시된다.
- [ ] 보스 HP 50%에서 페이즈2 대사가 뜨고 **2.5초 후 자동으로 사라지며, 그동안 공격/이동이 막히지 않는다**.
- [ ] 보스 처치 → 유언 → 3초 후 에필로그 3줄 → 엔딩 화면 순으로 끊김 없이 이어진다.
- [ ] 대사 ID를 일부러 오타 냈을 때 게임이 멈추지 않고 그 줄만 건너뛰며 로그에 경고가 남는다.
- [ ] 회귀 없음: 퀘스트 거절, 보상 지급, 상점/인벤토리 UI, 포탈 게이트가 기존대로 동작한다.
