# 8단계: 보스전 & 엔딩 — 구현 완료

## 목표

메인 퀘스트(나바쉬 토벌)를 수락한 플레이어가 **보스 맵으로 이동하는 포탈**을 타고 들어가, **페이즈가 있는 보스 나바쉬**와 싸운다. 보스를 처치하면 메인 퀘스트가 완료되고 **엔딩 화면**으로 게임이 마무리된다.

## 선행 단계

5단계 (메인 퀘스트/퀘스트 체인, `UQuestComponent`), 7단계 (패턴 시스템 — 보스에도 적용). 6단계의 연출(셰이크/히트스톱)도 사용.

## 구현된 파일

- `Monster.h/.cpp` — `bIsBoss` 플래그, `FOnBossDefeated` 전역 델리게이트 추가.
- `PortalPoint.h/.cpp` — `RequiredQuestID` 게이트 추가. (겸사겸사 기존 버그 수정: 오버랩 델리게이트 바인딩이 `Tick`에 있어 매 프레임 중복 등록되던 것을 `BeginPlay`로 이동)
- `Mon_Narbash.h/.cpp` — 페이즈 시스템(HP 50% 전환, 패턴 교체, 이속 증가, 전환 무적/연출), 보스 HP 바 생성, 7단계 학습 훅 연결.
- `UI_BossHpBar.h/.cpp` (신규) — 화면 상단 보스 HP 바 위젯.
- `UI_Ending.h/.cpp` (신규) — 엔딩 화면 위젯.
- `Py_Grey.h/.cpp` — 보스 처치 델리게이트 구독, 지연 후 엔딩 화면 표시.

## 설계 (실제 구현 기준 — 초안과 다른 점은 "초안과의 차이"에 정리)

### 1. 보스 맵 진입 게이트 — `APortalPoint`

```cpp
// PortalPoint.h
UPROPERTY(Category = "Portal", EditAnywhere)
FName RequiredQuestID;
```

`OnOverlapBegin`에서 `RequiredQuestID`가 `None`이 아니면 `Player->QuestComponent->GetActiveQuest().QuestID == RequiredQuestID`(그리고 `HasActiveQuest()`)를 검사, 불일치 시 이동하지 않고 `PrintViewport`로 "아직 들어갈 수 없다." 출력. 기존 포탈들은 `RequiredQuestID = None`이므로 회귀 없음.

### 2. 보스 나바쉬 — 페이즈 시스템

`AMon_Narbash`에 추가:

```cpp
UPROPERTY(EditAnywhere, Category="Boss") float Phase2HpRatio = 0.5f;
UPROPERTY(EditAnywhere, Category="Boss") TArray<UAnimMontage*> Phase2AttackPatterns;
UPROPERTY(EditAnywhere, Category="Boss") float Phase2SpeedMultiplier = 1.3f;
UPROPERTY(EditAnywhere, Category="Boss") UAnimMontage* PhaseTransitionMontage;
UPROPERTY(EditAnywhere, Category="Boss") TSubclassOf<UCameraShakeBase> PhaseTransitionCameraShake;
UPROPERTY(EditAnywhere, Category="Boss") float PhaseTransitionInvincibleDuration = 1.5f;
int32 CurrentPhase = 1;
```

`TakeDamage`를 오버라이드해서 `Super::TakeDamage` 호출 후 `GetHpRatio() <= Phase2HpRatio`이면 `EnterPhase2()`:

- `bPhaseTransitioning = true`로 전환 동안 무적 처리(`TakeDamage` 최상단에서 조기 반환) → `PhaseTransitionInvincibleDuration` 후 해제(타이머).
- `Phase2AttackPatterns`가 비어있지 않으면 `AttackPatterns`(7단계 필드, `AMonster` 소유)를 교체 — 밴딧 학습은 몬스터ID 기준으로 유지되므로 그대로 이어진다.
- 이동속도 `Phase2SpeedMultiplier` 배율 적용, 포효 몽타주 재생, 카메라 셰이크(6단계 재사용), 블랙보드 `Phase`(Int) 키 갱신(`AMonsterAIController::GetBlackboardComponent()`로 직접 접근 — `AAIController` 표준 API), 보스 HP 바에 페이즈 표시 갱신.
- 2페이즈 광역기는 코드 변경 없이 기존 파이프라인(`AEffect_Dot`/`AEffect_Projectile` + `UAnimNotify_SpawnSkillEffect`)을 몽타주에 연결하는 **에디터 작업**으로 처리 (신규 코드 불필요).
- 보스는 리스폰하지 않는다 — `pSpawnPoint`를 연결하지 않으면 `AMonster::~AMonster()`의 `pSpawnPoint->Respawn()` 호출 자체가 스킵된다(기존 로직 그대로).

### 3. 보스 HP 바 — `UUI_BossHpBar`

- `ProgressBar`(`HpBar`) + `TextBlock`(`PhaseText`), 둘 다 `BindWidgetOptional`. 보스 이름은 위젯 BP에 정적 텍스트로 넣으면 되므로 코드에서 다루지 않음.
- `AMon_Narbash::BeginPlay`에서 곧바로 생성·뷰포트에 추가(맵 진입 시 표시 — 초안의 두 옵션 중 더 단순한 쪽 채택), 머리 위 `HpBarComponent`는 `SetVisibility(false)`로 숨김. `EndPlay`에서 `RemoveFromParent`.

### 4. 보스 처치 → 메인 퀘스트 완료 → 엔딩

흐름 (실제 구현):

1. 나바쉬 사망 → `AMonster::TakeDamage`의 사망 분기에서 기존 `OnMonsterKilled.Broadcast(...)`에 이어 `bIsBoss`가 true면 `OnBossDefeated.Broadcast(MonsterID)`도 방송.
2. `UQuestComponent`는 그대로 동작 — 메인 퀘스트(체인의 마지막, Kill 목표 TargetID=나바쉬 MonsterID)는 **5단계 로직이 이미 즉시 자동 완료 처리**하므로 8단계에서 손댈 코드가 없다.
3. `APy_Grey`가 `AMonster::OnBossDefeated`를 구독(`BeginPlay`)해 `EndingDelay`(기본 3초) 후 `ShowEnding()` 호출.
4. `ShowEnding()`이 `UUI_Ending`을 생성해 뷰포트에 추가, `ExperienceComponent::GetCurrentLevel()`/`InventoryComponent::GetGold()`로 요약 텍스트를 채우고, 입력을 UI 전용으로 전환, `UGameplayStatics::SetGamePaused(true)`.
5. `UUI_Ending`의 [Restart] 버튼은 `SetGamePaused(false)` 후 `APy_Grey::StartLevelName`(에디터에서 지정)으로 `OpenLevel`, [Quit] 버튼은 `UKismetSystemLibrary::QuitGame`.

## 초안과의 차이 요약 (2026-07-17, 실제 코드 대조 후)

- **`bIsMainQuest` 필드는 존재하지 않았다** (05단계 완료 문서/코드 확인 결과, "자동 완료" 방식만 있고 메인/서브 구분 플래그는 없음). 초안은 이 필드가 이미 있다고 가정했으나 실제로는 없어서, 퀘스트 시스템과 완전히 분리된 **`AMonster::bIsBoss` → `OnBossDefeated` 전역 델리게이트** 방식으로 대체했다. 장점: 퀘스트 수락 여부와 무관하게 "보스가 죽으면 끝"이라는 게임 디자인 의도를 그대로 반영하고, 기존 처치 델리게이트(`OnMonsterKilled`)와 동일한 static 멀티캐스트 패턴이라 프로젝트 관례와도 맞는다. 엔딩 특칙(3번 항목의 "NPC 보고 없이 즉시 완료" 로직)도 이미 5단계에서 일반적으로 구현되어 있어 8단계에서 추가 코드가 필요 없었다.
- 보스 HP 바 표시 트리거는 "감지 시" 대신 "맵 진입(BeginPlay) 시"로 단순화 — 코드/에디터 작업 모두 최소화.
- `PortalPoint::Tick`에서 매 프레임 오버랩 델리게이트를 다시 바인딩하던 기존 버그를 `BeginPlay`로 옮겨 함께 고쳤다 (건드려야 하는 함수였고, 그대로 두면 보스 게이트 메시지가 프레임 수만큼 중복 출력됨).

## 에디터 작업 (사용자)

1. **보스 맵 제작**: 보스 아레나 레벨 생성, 나바쉬 배치(`bIsBoss = true`, `MonsterID = Narbash`), 조명/연출. 마을(또는 필드) 맵에 보스 맵행 `APortalPoint` 배치 후 `RequiredQuestID`를 나바쉬 퀘스트의 `QuestID`로, `TargetLevelName`을 보스 맵 이름으로 설정.
2. 나바쉬 BP: 1/2페이즈 공격 몽타주 배열(`AttackPatterns`/`Phase2AttackPatterns`), 포효 몽타주(`PhaseTransitionMontage`), 카메라 셰이크(`PhaseTransitionCameraShake`), `BossHpBarClass`(`WBP_BossHpBar`) 연결. 2페이즈 광역기 이펙트(`AEffect_Dot` 서브클래스 BP) 제작·노티파이 연결. 리스폰시키지 않으려면 `pSpawnPoint`를 연결하지 않고 레벨에 직접 배치.
3. `WBP_BossHpBar`(`HpBar` ProgressBar, `PhaseText` TextBlock), `WBP_Ending`(`SummaryText` TextBlock, `RestartButton`/`QuitButton` Button) 위젯 BP 생성 — 이름을 정확히 맞추면 `BindWidgetOptional`로 자동 연결됨.
4. `APy_Grey` BP에 `EndingWidgetClass = WBP_Ending`, `StartLevelName`(재시작 시 로드할 시작 레벨) 설정.
5. 나바쉬 BT에 블랙보드 `Phase`(Int) 키 추가 + 페이즈별 분기 브랜치(2페이즈에서 광역기/연속 공격 브랜치 활성화).

## 완료 기준 (PIE 테스트)

- [ ] 메인 퀘스트 수락 전에는 보스 포탈이 작동하지 않고, 수락 후에는 보스 맵으로 이동한다.
- [ ] 보스 맵에서 보스 HP 바가 상단에 표시된다.
- [ ] 보스 HP 50% 도달 시 포효 연출과 함께 2페이즈로 전환되고, 패턴이 더 공격적으로 바뀐다 (광역기 등장, 이동속도 증가).
- [ ] 보스 처치 시 잠시 후 엔딩 화면이 나오고, 버튼으로 게임을 재시작/종료할 수 있다.
- [ ] **전체 플로우 통합 테스트**: 새 게임 시작 → 채집 → 사냥 → 강화 퀘스트 체인 → 상점에서 포션 구비 → 메인 퀘스트 → 보스 처치 → 엔딩까지 끊김 없이 진행된다.
