# 5단계: 퀘스트 시스템 확장 (퀘스트 4종 체인)

## 목표

기존 퀘스트 시스템은 "처치 목표 1개"만 지원한다(`FQuestData::KillTargetID`/`KillTargetCount`, `UQuestComponent::HandleMonsterKilled`). 이를 **처치/채집/강화 3종 목표 타입**으로 일반화하고, 퀘스트 NPC 하나가 **4개 퀘스트를 순서대로** 내주는 체인 구조를 만든다.

퀘스트 체인 (README 기준):

| 순서 | 퀘스트 | 목표 타입 | 대상 | 수량 |
|---|---|---|---|---|
| 1 | 약초 채집 | Gather | `Herb` | 5 |
| 2 | 고블린 토벌 | Kill | 고블린 `MonsterID` | 5 |
| 3 | 무기 강화 | Enhance | (없음) | 1 |
| 4 | 나바쉬 토벌 | Kill | 나바쉬 `MonsterID` | 1 |

> **완료 방식은 "자동 완료"로 확정**: 목표 수량을 채우는 즉시 완료 처리 + 보상 지급한다(현재 구현된 고블린 토벌 퀘스트의 동작 그대로). NPC에게 다시 찾아가 "보고"하는 별도 단계는 두지 않는다 — 이미 배포된 처치 퀘스트 동작과 다른 방식을 새로 도입하면 회귀 위험만 커지고, 이 프로젝트의 "과설계 금지" 방침에도 맞지 않는다.

4번 퀘스트(나바쉬)는 이 단계에서 데이터와 진행도 추적만 갖추고, 실제 보스 페이즈/엔딩 연출은 8단계에서 별도로 붙인다. `AMonster::OnMonsterKilled`는 몬스터 종류와 무관하게 전역 방송되므로 이 단계에서 처치 목표 로직만으로 완결된다.

## 선행 단계

1단계(인벤토리), 2단계(`AGatherableObject::OnItemGathered`), 4단계(`APy_Grey::OnWeaponEnhanced`). 2단계 채집 퀘스트, 4단계 강화 퀘스트가 실제로 진행도로 연결되는 것은 이 단계에서 처음 이루어진다.

## 읽어야 할 파일

`GameInfo.h`(`FQuestData`/`FQuestReward`), `QuestComponent.h/.cpp`, `NPC_Quest.h/.cpp`, `UI_Quest.h/.cpp`, `UI_QuestTracker.h/.cpp`, `GatherableObject.h`(`OnItemGathered` 시그니처), `Py_Grey.h`(`OnWeaponEnhanced`/`WeaponLevel` 시그니처), `Monster.h`(`OnMonsterKilled` 시그니처).

## 설계

### 1. 목표 타입 일반화 — `GameInfo.h`

`FQuestData`의 `KillTargetID`/`KillTargetCount`/`CurrentKillCount`/`HasKillObjective()`를 아래로 교체한다 (필드명 변경이므로 사용하는 모든 곳을 함께 수정):

```cpp
UENUM(BlueprintType)
enum class EQuestObjectiveType : uint8
{
    None,       // 목표 없음 — 수락 즉시 보상 지급(기존 대화형 퀘스트와 동일)
    Kill,       // TargetID == AMonster::MonsterID
    Gather,     // TargetID == AItemBase::ItemID (AGatherableObject 포함)
    Enhance     // 무기 강화 성공 횟수. TargetID는 사용하지 않음(None으로 둔다)
};

USTRUCT(BlueprintType)
struct FQuestData
{
    GENERATED_BODY()

    // ... QuestID / QuestTitle / QuestDescription / Reward 는 기존 그대로 ...

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Objective")
    EQuestObjectiveType ObjectiveType = EQuestObjectiveType::None;

    // Kill: 몬스터 MonsterID / Gather: 아이템 ItemID / Enhance: 사용 안 함
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Objective")
    FName TargetID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Objective")
    int32 TargetCount = 0;

    // 진행도. 에디터에서 건드릴 값이 아니라 QuestComponent가 런타임에 갱신
    UPROPERTY(BlueprintReadOnly, Category = "Quest|Objective")
    int32 CurrentCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    bool bIsCompleted = false;

    bool HasObjective() const { return ObjectiveType != EQuestObjectiveType::None && TargetCount > 0; }
};
```

### 2. `UQuestComponent` — 델리게이트 3종 구독으로 일반화

기존에는 `AMonster::OnMonsterKilled` 하나만 구독했다. 여기에 `AGatherableObject::OnItemGathered`, `APy_Grey::OnWeaponEnhanced`를 같은 패턴으로 추가하고, 진행도 갱신 로직을 공통 함수 하나로 합친다.

```cpp
// QuestComponent.h
protected:
    void HandleMonsterKilled(FName MonsterID, int32 ExpReward, int32 GoldReward);
    void HandleItemGathered(FName ItemID, int32 Count);
    void HandleWeaponEnhanced(int32 NewLevel);

    // 목표 타입이 일치하고(+ Kill/Gather는 TargetID까지 일치) 진행 중이면 CurrentCount를 Amount만큼 올린다.
    // TargetCount에 도달하면 즉시 완료 처리 + 보상 지급.
    void TryAdvanceObjective(EQuestObjectiveType Type, FName InTargetID, int32 Amount);

    FDelegateHandle MonsterKilledHandle;
    FDelegateHandle ItemGatheredHandle;
    FDelegateHandle WeaponEnhancedHandle;
```

```cpp
// QuestComponent.cpp (핵심 로직)
void UQuestComponent::BeginPlay()
{
    Super::BeginPlay();
    MonsterKilledHandle   = AMonster::OnMonsterKilled.AddUObject(this, &UQuestComponent::HandleMonsterKilled);
    ItemGatheredHandle    = AGatherableObject::OnItemGathered.AddUObject(this, &UQuestComponent::HandleItemGathered);
    WeaponEnhancedHandle  = APy_Grey::OnWeaponEnhanced.AddUObject(this, &UQuestComponent::HandleWeaponEnhanced);
}

void UQuestComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    AMonster::OnMonsterKilled.Remove(MonsterKilledHandle);
    AGatherableObject::OnItemGathered.Remove(ItemGatheredHandle);
    APy_Grey::OnWeaponEnhanced.Remove(WeaponEnhancedHandle);
    Super::EndPlay(EndPlayReason);
}

void UQuestComponent::HandleMonsterKilled(FName MonsterID, int32, int32)
{
    TryAdvanceObjective(EQuestObjectiveType::Kill, MonsterID, 1);
}

void UQuestComponent::HandleItemGathered(FName ItemID, int32 Count)
{
    TryAdvanceObjective(EQuestObjectiveType::Gather, ItemID, Count);
}

void UQuestComponent::HandleWeaponEnhanced(int32 NewLevel)
{
    TryAdvanceObjective(EQuestObjectiveType::Enhance, NAME_None, 1);
}

void UQuestComponent::TryAdvanceObjective(EQuestObjectiveType Type, FName InTargetID, int32 Amount)
{
    if (!bHasActiveQuest || ActiveQuest.bIsCompleted) return;
    if (ActiveQuest.ObjectiveType != Type) return;

    // Kill/Gather는 대상 ID까지 일치해야 함. Enhance는 대상 ID 없이 횟수만 센다.
    if (Type != EQuestObjectiveType::Enhance)
    {
        if (ActiveQuest.TargetID.IsNone() || ActiveQuest.TargetID != InTargetID) return;
    }

    ActiveQuest.CurrentCount = FMath::Min(ActiveQuest.CurrentCount + Amount, ActiveQuest.TargetCount);

    if (ActiveQuest.CurrentCount >= ActiveQuest.TargetCount)
    {
        ActiveQuest.bIsCompleted = true;
        GrantReward();
    }

    OnQuestTrackerChanged.Broadcast();
}
```

`AcceptQuest`도 `ObjectiveType::None`(목표 없는 대화형 퀘스트)일 때 수락 즉시 완료 처리하도록 흡수한다:

```cpp
void UQuestComponent::AcceptQuest(const FQuestData& InQuestData)
{
    ActiveQuest = InQuestData;
    ActiveQuest.CurrentCount = 0;
    ActiveQuest.bIsCompleted = false;
    bHasActiveQuest = true;

    if (!ActiveQuest.HasObjective())
    {
        ActiveQuest.bIsCompleted = true;
        GrantReward();
    }

    OnQuestTrackerChanged.Broadcast();
}
```

이렇게 하면 `ANPC_Quest::HandleQuestAccepted`에 있던 "목표 없으면 즉시 보상 지급" 분기(인벤토리/골드/경험치 직접 호출)가 필요 없어진다 — 보상 지급 경로가 `QuestComponent::GrantReward` 하나로 통일된다.

### 3. `ANPC_Quest` — 단일 퀘스트에서 체인으로

기존 `FQuestData QuestData;` 하나를 배열로 바꾸고, "직전에 낸 체인 퀘스트가 방금 완료됐는지"를 상호작용 시점에 확인해서 다음 칸으로 넘어간다. 별도 델리게이트 구독 없이 `Interact_Implementation`에서 그때그때 확인하는 방식이라 상태 관리가 단순하다.

```cpp
// NPC_Quest.h
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
TArray<FQuestData> QuestChain;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
int32 CurrentChainIndex = 0;
```

```cpp
// NPC_Quest.cpp
void ANPC_Quest::Interact_Implementation(AActor* Interactor)
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
```

`HandleQuestDeclined`/`RestoreInputMode`는 그대로 유지 (변경 없음). 거절 시 `CurrentChainIndex`가 그대로이므로 다음 상호작용 때 같은 퀘스트를 다시 제안한다.

### 4. `UUI_QuestTracker` — 목표 타입별 진행 문구

`HasKillObjective()`/`CurrentKillCount`/`KillTargetCount` 참조를 `HasObjective()`/`CurrentCount`/`TargetCount`로 바꾸고, 타입에 따라 동사만 다르게 표시한다:

```cpp
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
```

`UUI_Quest`(수락/거절 다이얼로그)는 `FQuestData`를 그대로 표시만 하므로 코드 변경 없음.

## 에디터 작업 (사용자)

1. 기존에 배치된 퀘스트 NPC(고블린 토벌 퀘스트를 내주던 것)의 `QuestData` 단일 값 설정을 **`QuestChain` 배열**로 옮긴다. 순서대로 4항목 입력:
   - `[0]` 약초 채집 — `ObjectiveType=Gather`, `TargetID=Herb`, `TargetCount=5`
   - `[1]` 고블린 토벌 — `ObjectiveType=Kill`, `TargetID=`(고블린 BP의 `MonsterID`값), `TargetCount=5` (기존 값 그대로 이전)
   - `[2]` 무기 강화 — `ObjectiveType=Enhance`, `TargetID=None`, `TargetCount=1`
   - `[3]` 나바쉬 토벌 — `ObjectiveType=Kill`, `TargetID=`(나바쉬 BP의 `MonsterID`값), `TargetCount=1`
   - 각 항목의 `QuestTitle`/`QuestDescription`/`Reward`(아이템/골드/경험치)도 함께 채운다.
2. 퀘스트를 여러 NPC로 나누고 싶으면(예: 채집은 마을 초입 NPC, 강화는 대장장이 옆 NPC) NPC를 추가 배치하고 각자의 `QuestChain`에 1항목씩만 넣어도 된다 — 구조상 문제 없음. 다만 이 경우도 **한 번에 진행 가능한 퀘스트는 항상 1개**이므로, 순서를 지키지 않고 여러 NPC에게 동시에 말을 걸어도 두 번째부터는 대화가 열리지 않는다(정상 동작).
3. 4번 나바쉬 퀘스트는 8단계에서 보스 맵/포탈이 준비되기 전까지는 실제로 완료하기 어려울 수 있다 — 데이터만 미리 넣어두고 PIE 테스트는 1~3번 위주로 진행.

## 완료 기준 (PIE 테스트)

- [ ] 채집 퀘스트 수락 후 약초를 5개 채집하면 자동으로 완료되고 보상이 지급된다 (채집 퀘스트 수락 전에 채집한 것은 카운트되지 않는다).
- [ ] 채집 퀘스트 완료 후 같은 NPC에게 다시 말을 걸면 고블린 토벌 퀘스트가 제안된다.
- [ ] 고블린 5마리 처치 → 완료 → 다시 말 걸면 무기 강화 퀘스트가 제안된다.
- [ ] 무기 강화 퀘스트 수락 후 대장장이에게서 1회 강화하면 퀘스트가 자동 완료된다.
- [ ] 진행 중인 퀘스트가 있을 때 다른 퀘스트 NPC(또는 같은 NPC)에게 말을 걸어도 새 대화가 열리지 않는다.
- [ ] 좌측 퀘스트 트래커에 목표 타입에 맞는 문구("채집 N/M", "처치 N/M", "강화 N/M")가 표시된다.
- [ ] 기존 기능 회귀 없음: 퀘스트 거절 시 같은 퀘스트가 다시 제안되고, 보상(아이템/골드/경험치) 지급이 정상 동작한다.
