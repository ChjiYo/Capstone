# 8단계: 보스전 & 엔딩

## 목표

메인 퀘스트(나바쉬 토벌)를 수락한 플레이어가 **보스 맵으로 이동하는 포탈**을 타고 들어가, **페이즈가 있는 보스 나바쉬**와 싸운다. 보스를 처치하면 메인 퀘스트가 완료되고 **엔딩 화면**으로 게임이 마무리된다.

## 선행 단계

5단계 (메인 퀘스트/`bIsMainQuest`, `UQuestComponent`), 7단계 (패턴 시스템 — 보스에도 적용). 6단계의 연출(셰이크/히트스톱)도 사용.

## 읽어야 할 파일

`Mon_Narbash.h/.cpp`, `Mon_NarbashAnim.h/.cpp`, `Monster.h/.cpp`, `PortalPoint.h/.cpp`, `QuestComponent.h`, `UI_HpBar.h/.cpp`, `GM_Main.h/.cpp`.

## 설계

### 1. 보스 맵 진입 게이트 — `APortalPoint` 확장

```cpp
// APortalPoint에 추가
// None이면 기존처럼 무조건 통과. 지정 시 플레이어의 활성 퀘스트가 이 ID일 때만 작동
UPROPERTY(EditAnywhere, Category="Portal") FName RequiredQuestID;
```

- 오버랩 시 `UQuestComponent::GetActiveQuest().QuestID == RequiredQuestID` 검사. 불일치 시 이동하지 않고 안내 문구(`PrintViewport` 또는 간단 위젯): "아직 들어갈 수 없다".
- 기존 포탈들은 `RequiredQuestID = None`이므로 회귀 없음.

### 2. 보스 나바쉬 — 페이즈 시스템

`AMon_Narbash`를 보스로 승격한다 (별도 클래스 신설 없이 기존 클래스 확장).

```cpp
// AMon_Narbash에 추가
UPROPERTY(EditAnywhere, Category="Boss") float Phase2HpRatio = 0.5f;  // HP 50% 이하 → 2페이즈
UPROPERTY(EditAnywhere, Category="Boss") TArray<UAnimMontage*> Phase2AttackPatterns; // 2페이즈 전용 패턴
UPROPERTY(EditAnywhere, Category="Boss") float Phase2SpeedMultiplier = 1.3f;  // 이속/공속 배율
int32 CurrentPhase = 1;
```

- **페이즈 전환** (`TakeDamage`에서 HP 비율 검사):
  - 전환 연출: 포효 몽타주 재생(에디터 연결) + 강한 카메라 셰이크(6단계 재사용) + 전환 중 무적.
  - 블랙보드 `Phase`(Int) 키 갱신 → BT가 페이즈별 브랜치 분기(2페이즈에서 광역기/연속 공격 브랜치 활성).
  - 패턴 교체: 7단계의 `AttackPatterns`를 `Phase2AttackPatterns`로 스왑 (밴딧 학습도 그대로 적용됨 — 보스 역시 플레이어 패턴에 적응).
- **2페이즈 광역기**: 기존 `AEffect_Dot`(장판) 또는 `AEffect_Projectile`을 `UAnimNotify_SpawnSkillEffect`로 몽타주에 연결 (기존 파이프라인 그대로, 신규 코드 최소).
- 보스는 리스폰하지 않는다: 보스용 `ASpawnPoint`를 쓰지 않고 맵에 직접 배치하거나, 스폰 포인트의 유한 모드 사용.

### 3. 보스 HP 바 — `UUI_BossHpBar`

- 화면 상단 와이드 바: 보스 이름 + HP ProgressBar + 페이즈 표시(선택). `BindWidgetOptional`.
- 표시 트리거: 보스 맵에서 보스가 플레이어를 감지했을 때(또는 맵 진입 시) 뷰포트에 추가, 보스 사망 시 제거.
- 소유 위치: 보스 맵 전용 로직이므로 `AMon_Narbash`가 위젯 클래스를 들고 생성하는 것이 가장 단순 (GameMode 분리 불필요). 머리 위 기존 `HpBarComponent`는 보스에서 숨김.

### 4. 보스 처치 → 메인 퀘스트 완료 → 엔딩

흐름:

1. 나바쉬 사망 → 기존 `OnMonsterKilled.Broadcast("Narbash", ...)` (이미 동작).
2. `UQuestComponent`가 메인 퀘스트 `CurrentCount` 충족 처리 (5단계 로직 그대로).
3. **엔딩 특칙**: 메인 퀘스트(`bIsMainQuest`)는 NPC 보고 없이 **즉시 완료** 처리하고, `DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMainQuestCompleted)`를 방송.
4. `APy_Grey`(또는 `GM_Main`)가 이를 구독 → 3초 지연(사망 연출 감상) 후 `UUI_Ending` 표시.

`UUI_Ending` 구성 (`BindWidgetOptional`):

- "THE END" / 클리어 문구, 플레이 결과 요약(최종 레벨, 처치 수, 골드 — `ExperienceComponent`/`InventoryComponent`에서 조회, 항목은 선택), [메인으로] 버튼(시작 레벨 다시 로드) 또는 [종료] 버튼(`UKismetSystemLibrary::QuitGame`).
- 표시 중 입력 모드 UI 전환 + 게임 일시정지(`SetGamePaused`).

## 에디터 작업 (사용자)

1. **보스 맵 제작**: 보스 아레나 레벨 생성, 나바쉬 배치, 조명/연출. 마을(또는 필드) 맵에 보스 맵행 `APortalPoint` 배치 후 `RequiredQuestID = Q_Main_Narbash`, `TargetLevelName = 보스 맵 이름` 설정.
2. 나바쉬 BP: `MonsterID = Narbash` 확인, 1/2페이즈 공격 몽타주 배열, 포효 몽타주, 페이즈 수치 설정. 2페이즈 광역기 이펙트(`AEffect_Dot` 서브클래스 BP) 제작·노티파이 연결.
3. `WBP_BossHpBar`, `WBP_Ending` 위젯 BP 생성·연결.
4. 나바쉬 BT에 페이즈 분기 브랜치 추가 (블랙보드 `Phase` 키).

## 완료 기준 (PIE 테스트)

- [ ] 메인 퀘스트 수락 전에는 보스 포탈이 작동하지 않고, 수락 후에는 보스 맵으로 이동한다.
- [ ] 보스 맵에서 보스 HP 바가 상단에 표시된다.
- [ ] 보스 HP 50% 도달 시 포효 연출과 함께 2페이즈로 전환되고, 패턴이 더 공격적으로 바뀐다 (광역기 등장).
- [ ] 보스 처치 시 잠시 후 엔딩 화면이 나오고, 버튼으로 게임을 재시작/종료할 수 있다.
- [ ] **전체 플로우 통합 테스트**: 새 게임 시작 → 채집 → 사냥 → 강화 퀘스트 체인 → 상점에서 포션 구비 → 메인 퀘스트 → 보스 처치 → 엔딩까지 끊김 없이 진행된다.
