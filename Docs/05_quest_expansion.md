# 5단계: 퀘스트 시스템 확장 (퀘스트 4종 체인)

## 목표

현재 "처치 퀘스트 1개"만 지원하는 퀘스트 시스템을 확장한다.

1. 퀘스트 목표 타입 일반화: **처치(Kill) / 채집(Gather) / 강화(Enhance)**.
2. 한 NPC가 **퀘스트를 순서대로(체인)** 제공: 채집 → 사냥 → 강화 → 메인(보스).
3. 완료된 퀘스트는 NPC에게 **돌아가서 보고**하면 보상 지급 + 다음 퀘스트 개방.

## 선행 단계

2단계 (`AGatherableObject::OnItemGathered`), 4단계 (`APy_Grey::OnWeaponEnhanced`). 처치 이벤트는 기존 `AMonster::OnMonsterKilled` 사용.

## 읽어야 할 파일

`GameInfo.h` (`FQuestData`/`FQuestReward`), `QuestComponent.h/.cpp`, `NPC_Quest.h/.cpp`, `UI_Quest.h/.cpp`, `UI_QuestTracker.h/.cpp`.

## 설계

### 1. `FQuestData` 확장 (`GameInfo.h`)

```cpp
UENUM(BlueprintType)
enum class EQuestObjectiveType : uint8
{
    Kill,      // TargetID = AMonster::MonsterID
    Gather,    // TargetID = 아이템 ItemID (DT_Items RowName)
    Enhance    // TargetID 미사용, TargetCount = 강화 횟수
};

// FQuestData에 추가/변경
EQuestObjectiveType ObjectiveType = EQuestObjectiveType::Kill;
FName TargetID;          // 기존 KillTargetID를 일반화
int32 TargetCount = 0;   // 기존 KillTargetCount를 일반화
int32 CurrentCount = 0;  // 기존 CurrentKillCount를 일반화 (BlueprintReadOnly 유지)
bool bIsMainQuest = false;  // 메인 퀘스트 표시(트래커 강조/보스 포탈 게이트용)
```

- **마이그레이션 방침**: `KillTargetID`/`KillTargetCount`/`CurrentKillCount` 필드를 위 일반 필드로 **이름 변경**한다 (필드 중복 유지 금지). BP(`BP_UI_NPC_Quest` 등)에 저장된 기존 값은 사라지므로 에디터에서 재입력 필요 — "에디터 작업"에 명시.
- `HasKillObjective()` → `HasObjective() const { return TargetCount > 0; }`로 교체하고 호출부 갱신.

### 2. `UQuestComponent` 확장

활성 퀘스트 1개 모델은 유지한다 (동시 다중 퀘스트는 과설계 — 체인 구조상 불필요).

- 구독 확장: `BeginPlay`에서 3개 이벤트 모두 구독, `EndPlay`에서 모두 해제.
  - `AMonster::OnMonsterKilled` → `ObjectiveType == Kill && MonsterID == TargetID`일 때 `CurrentCount++`.
  - `AGatherableObject::OnItemGathered(ItemID, Count)` → `Gather && ItemID == TargetID`일 때 `CurrentCount += Count`.
  - `APy_Grey::OnWeaponEnhanced` → `Enhance`일 때 `CurrentCount++`.
- 상태 추가: 목표 달성(`CurrentCount >= TargetCount`) 시 `bIsCompleted = true`로 바꾸되 **보상은 즉시 지급하지 않는다**. 트래커에 "완료 — NPC에게 보고" 표시.
  - `UFUNCTION(BlueprintPure) bool IsQuestReadyToTurnIn() const`
  - `UFUNCTION(BlueprintCallable) void TurnInQuest()` — NPC가 호출. 보상 지급(기존 `GrantReward` 재사용: 아이템→인벤토리, `RewardExp`→`ExperienceComponent::AddExp`, `RewardGold`→`AddGold`) 후 활성 퀘스트 해제, `OnQuestTrackerChanged` 방송.
  - **Gather 퀘스트 특칙**: 보고 시점에 인벤토리에서 `TargetID` 아이템을 `TargetCount`만큼 회수(`RemoveItem`)한다. 수집 후 팔아버렸으면 `CurrentCount`가 충족돼도 보고 불가 — 단순화를 위해 진행도 자체를 `GetItemCount` 기준으로 다시 검사하는 방식이 안전하다 (`OnInventoryChanged` 구독으로 트래커 갱신).
- 기존 "수락 즉시 보상 지급" 경로가 남아있다면 제거하고, 보상은 오직 `TurnInQuest`에서만.

### 3. `ANPC_Quest` — 퀘스트 체인

```cpp
// 단일 QuestData → 배열로 변경
UPROPERTY(EditAnywhere, Category="Quest") TArray<FQuestData> QuestChain;
UPROPERTY(VisibleAnywhere, Category="Quest") int32 CurrentQuestIndex = 0;
```

상호작용(`Interact_Implementation`) 분기:

1. 플레이어의 활성 퀘스트가 이 NPC 것이고 `IsQuestReadyToTurnIn()` → `TurnInQuest()` 호출 + 완료 다이얼로그(간단 문구) + `CurrentQuestIndex++`.
2. 활성 퀘스트가 있으나 미완료 → "진행 중" 안내 문구만 (UI_Quest 재사용, 수락 버튼 숨김 또는 안내 전용 모드).
3. 활성 퀘스트 없음 && `CurrentQuestIndex < QuestChain.Num()` → 기존 수락/거절 다이얼로그로 `QuestChain[CurrentQuestIndex]` 제안.
4. 체인 소진 → "모든 의뢰 완료" 문구.

`UUI_Quest`에 완료 보고/진행 중 안내를 위한 모드 구분이 필요하다 (enum 파라미터 하나로 처리, 위젯 요소는 `BindWidgetOptional`).

### 4. `UUI_QuestTracker` 표시 확장

- 목표 텍스트를 타입별로 생성: Kill "고블린 토벌 3/5", Gather "약초 5/5 (보고 가능)", Enhance "무기 강화 0/1".
- `bIsCompleted`(보고 대기) 상태를 색/문구로 구분. `bIsMainQuest`면 제목 강조 (선택).

### 5. 퀘스트 데이터 (NPC BP에 입력할 체인)

| # | QuestID | 제목 | Objective | TargetID | Count | 보상(안) |
|---|---|---|---|---|---|---|
| 0 | `Q_Gather_Herb` | 약초가 필요해 | Gather | `Herb` | 5 | `HpPotion` x2, Exp 50, Gold 30 |
| 1 | `Q_Hunt_Goblin` | 고블린 토벌 | Kill | `Goblin` (기존 MonsterID 값 확인) | 5 | Exp 100, Gold 80 |
| 2 | `Q_Enhance_Weapon` | 무기를 벼려라 | Enhance | - | 1 | Exp 100, Gold 50 |
| 3 | `Q_Main_Narbash` | 나바쉬 토벌 (메인) | Kill | `Narbash` | 1 | Exp 500 (엔딩은 8단계) |

`bIsMainQuest = true`는 3번만. `TargetID`는 실제 몬스터 BP의 `MonsterID` 값과 반드시 일치시켜야 한다 (에디터에서 확인).

## 에디터 작업 (사용자)

1. 퀘스트 NPC BP의 기존 `QuestData` 값이 필드 개편으로 초기화됨 → `QuestChain` 배열에 위 4개 퀘스트 재입력.
2. 고블린/나바쉬 BP의 `MonsterID`가 표의 `TargetID`와 일치하는지 확인.
3. `WBP_QuestTracker`/`WBP_UI_NPC_Quest`에 새 표시 요소(진행 중/보고 대기 문구, 완료 보고 버튼 등) 추가 — C++ `BindWidgetOptional` 이름에 맞출 것.

## 완료 기준 (PIE 테스트)

- [ ] NPC에게 첫 퀘스트(약초 채집)를 받고, 약초 5개 채집 시 트래커가 5/5 + "보고 가능"으로 바뀐다.
- [ ] NPC에게 돌아가면 보상(포션 2, 경험치, 골드)이 지급되고 약초 5개가 회수되며, 다음 퀘스트(고블린 토벌)를 제안받는다.
- [ ] 고블린 5마리 처치 → 보고 → 강화 퀘스트 수락 → 강화 1회 → 보고, 까지 체인이 끊기지 않고 이어진다.
- [ ] 메인 퀘스트(나바쉬)가 마지막에 개방된다 (완료는 8단계에서).
- [ ] 진행 중 상태에서 NPC와 대화하면 "진행 중" 안내만 나오고 퀘스트가 중복 수락되지 않는다.
