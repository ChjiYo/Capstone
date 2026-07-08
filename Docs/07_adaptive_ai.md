# 7단계: 적응형 몬스터 AI (플레이어 패턴 학습)

## 목표

몬스터가 **플레이어의 행동 패턴을 관측·누적**하고, 그에 따라 **공격 패턴 선택과 교전 방식이 유동적으로 변하게** 한다.

접근 방식: 신경망/외부 ML 라이브러리 없이, **런타임 통계 + ε-greedy 밴딧(bandit) 학습**으로 구현한다.
이 방식은 (1) UE C++만으로 구현 가능하고, (2) 시연에서 "학습 전/후" 차이를 눈으로 보여줄 수 있으며, (3) 캡스톤 발표에서 "온라인 강화학습(멀티암드 밴딧) 기반 적응형 AI"로 설명할 수 있다.

## 선행 단계

6단계 (`APy_Grey::OnPlayerDodge` 훅, 무적 회피). 기존 BT 노드 구조 사용.

## 읽어야 할 파일

`MonsterAIController.h/.cpp`, `BTService_Detect.h/.cpp`, `BTTask_Attack.h/.cpp`, `BTDecorator_AttackDist.h/.cpp`, `Monster.h/.cpp`, `Mon_Goblin.h/.cpp` (무기 박스 명중 판정 위치), `Py_Grey.h` (6단계 결과물).

## 설계

### 전체 그림

```
[관측]  플레이어 회피/거리/포션 사용  ──┐
[관측]  몬스터 공격 패턴별 명중/회피됨 ──┤→  UCombatPatternSubsystem (GameInstance 수명, 맵 이동에도 유지)
                                        │      · 패턴별 명중률 가중치 (ε-greedy 밴딧)
                                        │      · 플레이어 성향 통계 (회피율, 평균 교전 거리, 회복 습관)
[활용]  UBTService_PatternAnalysis  ←──┘
           └→ 블랙보드 키 기록 (SelectedPattern, FeintChance, Aggression)
                 └→ UBTTask_Attack이 패턴 인덱스로 몽타주 선택 / 페인트(속임수) 실행
```

### 1. `UCombatPatternSubsystem : UGameInstanceSubsystem`

몬스터가 죽고 리스폰돼도 학습이 유지되도록 **GameInstance 수명**의 서브시스템에 둔다. 저장 파일(SaveGame) 영속화는 범위 제외 — 한 게임 세션 내 학습만.

```cpp
// 몬스터 종류(MonsterID)별 학습 상태
USTRUCT()
struct FPatternStats
{
    TArray<float> PatternWeights;   // 공격 패턴별 성공 추정치 (0~1), 크기 = 패턴 수
    int32 TotalAttacks = 0;
};

class UCombatPatternSubsystem : public UGameInstanceSubsystem
{
public:
    // ---- 기록 (몬스터/플레이어 코드가 호출) ----
    void RecordAttackResult(FName MonsterID, int32 PatternIndex, bool bHit);
    void RecordPlayerDodge();                  // APy_Grey::OnPlayerDodge 구독으로 자동 기록
    void RecordPlayerPotionUse(float HpRatio); // 포션 사용 시점의 HP 비율
    void RecordEngageDistance(float Dist);     // BTService_Detect 틱에서 샘플링

    // ---- 조회 (BT 서비스가 호출) ----
    int32 SelectAttackPattern(FName MonsterID, int32 NumPatterns);  // ε-greedy
    float GetDodgeRate() const;          // 몬스터 공격 대비 회피 빈도 (0~1)
    float GetAvgEngageDistance() const;
    float GetHealHpRatio() const;        // 플레이어가 보통 회복하는 HP 비율

protected:
    TMap<FName, FPatternStats> StatsByMonster;
    // 회피/거리/회복 통계는 EMA(지수이동평균)로 유지 — 최근 행동에 더 민감
};
```

**학습 규칙 (ε-greedy + EMA)**

- 가중치 갱신: `W[p] = (1 - Alpha) * W[p] + Alpha * (bHit ? 1 : 0)`, `Alpha = 0.3` (에디터/ini 노출).
- 패턴 선택: 확률 `Epsilon = 0.2`로 무작위 패턴(탐험), 나머지는 `W`가 최대인 패턴(활용). 초기 가중치는 전부 0.5.
- 회피율 등 성향 통계도 같은 EMA 방식: `DodgeRate = (1-Alpha)*DodgeRate + Alpha*(회피했으면 1)`.

### 2. 관측 훅 연결 (기록이 들어오는 곳)

| 이벤트 | 훅 위치 | 기록 |
|---|---|---|
| 몬스터 공격 명중 | 몬스터 무기 박스 오버랩 → `TakeDamage` 성공 시 | `RecordAttackResult(ID, Pattern, true)` |
| 몬스터 공격 빗나감/회피됨 | `UBTTask_Attack` 종료 시점에 명중 플래그가 false면 | `RecordAttackResult(ID, Pattern, false)` |
| 플레이어 회피 | `APy_Grey::OnPlayerDodge` (6단계) 구독 | `RecordPlayerDodge()` |
| 포션 사용 | 포션 사용 코드(1단계)에서 호출 | `RecordPlayerPotionUse(현재 HP 비율)` |
| 교전 거리 | `UBTService_Detect` 틱에서 타겟 존재 시 | `RecordEngageDistance(dist)` |

`AMonster`는 "이번 공격의 패턴 인덱스"와 "이번 공격이 명중했는가" 플래그를 들고 있어야 한다 → `int32 CurrentPatternIndex; bool bCurrentAttackHit;` 추가, 공격 시작 시 리셋.

### 3. 몬스터 공격 패턴 다양화

현재 `UBTTask_Attack`은 단일 공격 몽타주 재생으로 추정된다. 확장:

- `AMonster`에 `UPROPERTY(EditAnywhere, Category="AI") TArray<UAnimMontage*> AttackPatterns;` — 패턴 = 몽타주 (예: 빠른 공격 / 느린 강공격 / 돌진 공격). 비어 있으면 기존 단일 공격으로 폴백 (다른 몬스터 회귀 방지).
- `UBTTask_Attack` 수정: 실행 시 블랙보드의 `SelectedPattern` 인덱스 몽타주를 재생.

### 4. `UBTService_PatternAnalysis : UBTService` (신규)

몬스터 BT의 루트/전투 브랜치에 붙는 서비스. 틱(0.5초 간격)마다:

1. `SelectAttackPattern` 호출 → 블랙보드 `SelectedPattern` (int) 기록.
2. 성향 → 행동 변조 값 계산 후 블랙보드 기록:

| 관측된 플레이어 성향 | 블랙보드 키 | 몬스터 행동 변화 |
|---|---|---|
| 회피율 높음 (`DodgeRate > 0.5`) | `FeintChance` ↑ | 공격 전 **페인트**: 공격 모션 시작 딜레이를 무작위로 넣어 회피 타이밍을 흐트러뜨림 (`UBTTask_Attack`에서 `FeintChance` 확률로 0.3~0.6초 대기 후 공격) |
| 원거리 유지 성향 (`AvgEngageDistance` 큼) | `Aggression` ↑ | 이동 속도 배율 증가 + `UBTTask_Dash` 브랜치 우선 (거리 좁히기) |
| 저HP에 회복 습관 (`HealHpRatio` 파악됨) | `Aggression` ↑ (플레이어 HP가 그 비율 근처일 때) | 플레이어가 회복하려는 타이밍에 몰아붙임 |

- 임계값/배율은 전부 서비스의 `UPROPERTY(EditAnywhere)`로 노출.
- 블랙보드 키 3개 추가 필요: `SelectedPattern`(Int), `FeintChance`(Float), `Aggression`(Float) — 에디터 작업.

### 5. 시연 가시화 (발표용, 선택이지만 강력 추천)

- 디버그 HUD 또는 `PrintViewport`로 현재 학습 상태 출력 토글 (콘솔 명령 `CVar` 또는 키): 패턴별 가중치, 플레이어 회피율.
- 시연 시나리오: "계속 구르기만 하는 플레이어" → 몇 번의 교전 후 몬스터가 페인트를 섞기 시작하는 것을 보여줌.

## 에디터 작업 (사용자)

1. 고블린(및 원하는 몬스터) BT에 `UBTService_PatternAnalysis` 부착, 블랙보드에 `SelectedPattern`/`FeintChance`/`Aggression` 키 추가.
2. 고블린 BP `AttackPatterns`에 공격 몽타주 2~3개 연결 (기존 콤보 몽타주 분할 재사용 가능).
3. `UBTTask_Attack` 노드 설정에서 패턴/페인트 관련 노출 값 확인.

## 완료 기준 (PIE 테스트)

- [ ] 같은 고블린과 반복 교전 시 공격 패턴(몽타주)이 다양하게 나온다.
- [ ] 회피만 반복하는 플레이 스타일로 몇 번 싸우면, 몬스터가 공격 타이밍을 늦추는 페인트를 섞기 시작한다 (디버그 출력으로 `FeintChance` 상승 확인).
- [ ] 멀리서 도망 다니면 몬스터 추격이 공격적으로 변한다 (`Aggression` 상승 확인).
- [ ] 몬스터가 리스폰돼도 학습 상태가 유지된다 (서브시스템 수명 확인).
- [ ] `AttackPatterns`를 설정하지 않은 몬스터(라바 등)는 기존과 동일하게 동작한다 (회귀 없음).
