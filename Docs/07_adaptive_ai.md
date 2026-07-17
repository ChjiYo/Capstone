# 7단계: 적응형 몬스터 AI (플레이어 패턴 학습) — 구현 완료

## 목표

몬스터가 **플레이어의 행동 패턴을 관측·누적**하고, 그에 따라 **공격 패턴 선택과 교전 방식이 유동적으로 변하게** 한다.

접근 방식: 신경망/외부 ML 라이브러리 없이, **런타임 통계 + ε-greedy 밴딧(bandit) 학습**으로 구현했다.
이 방식은 (1) UE C++만으로 구현 가능하고, (2) 시연에서 "학습 전/후" 차이를 눈으로 보여줄 수 있으며, (3) 캡스톤 발표에서 "온라인 강화학습(멀티암드 밴딧) 기반 적응형 AI"로 설명할 수 있다.

## 선행 단계

6단계 (`APy_Grey::OnPlayerDodge` 훅, 무적 회피) — 완료됨. 기존 BT 노드 구조 사용.

## 구현된 파일

- `CombatPatternSubsystem.h/.cpp` (신규) — `UGameInstanceSubsystem`. 학습 상태 보관 + ε-greedy 로직.
- `BTService_PatternAnalysis.h/.cpp` (신규) — BT 서비스. 서브시스템 조회 → 블랙보드에 반영.
- `Monster.h` — `TArray<UAnimMontage*> AttackPatterns`, `int32 CurrentPatternIndex` 추가.
- `BTTask_Attack.h/.cpp` — 패턴 인덱스로 몽타주 선택 + 페인트 지연 실행.
- `BTService_Detect.cpp` — 교전 거리 샘플링 훅 추가.
- `Weapon.cpp` — 몬스터 공격 명중/회피(무적 프레임) 결과 기록 훅 추가.
- `Py_Grey.cpp` (`UsePotionItem`) — 포션 사용 시점 HP 비율 기록 훅 추가.
- `CharBase.h` — `GetHpRatio()` public getter 추가 (회복 습관 판정에 필요, 기존에 없었음).

## 설계 (실제 구현 기준 — 초안과 다른 점은 "초안과의 차이"에 정리)

### 1. `UCombatPatternSubsystem : UGameInstanceSubsystem`

몬스터가 죽고 리스폰돼도 학습이 유지되도록 GameInstance 수명 서브시스템에 둔다. SaveGame 영속화는 범위 제외(세션 내 학습만).

```cpp
USTRUCT()
struct FPatternStats
{
    TArray<float> PatternWeights;   // 공격 패턴별 성공 추정치 (0~1)
    int32 TotalAttacks = 0;
};

class UCombatPatternSubsystem : public UGameInstanceSubsystem
{
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override; // APy_Grey::OnPlayerDodge 구독
    virtual void Deinitialize() override;                                  // 구독 해제

    void RecordAttackResult(FName MonsterID, int32 PatternIndex, bool bHit);
    void RecordPlayerDodge();
    void RecordPlayerPotionUse(float HpRatio);
    void RecordEngageDistance(float Dist);

    int32 SelectAttackPattern(FName MonsterID, int32 NumPatterns); // ε-greedy, NumPatterns<=0이면 -1
    float GetDodgeRate() const;
    float GetAvgEngageDistance() const;
    bool GetHealHpRatio(float& OutRatio) const; // 샘플이 없으면 false

    void PrintDebugStats(FName MonsterID) const; // 시연용 화면 출력

protected:
    UPROPERTY(EditAnywhere) float Alpha = 0.3f;
    UPROPERTY(EditAnywhere) float Epsilon = 0.2f;
    TMap<FName, FPatternStats> StatsByMonster;
};
```

**학습 규칙 (ε-greedy + EMA)**

- 가중치 갱신: `W[p] = (1 - Alpha) * W[p] + Alpha * (bHit ? 1 : 0)`. 초기 가중치는 0.5.
- 패턴 선택: 확률 `Epsilon`으로 무작위 패턴(탐험), 나머지는 `W`가 최대인 패턴(활용).
- 회피율/평균 교전 거리도 같은 EMA 방식으로 누적. 회복 습관은 EMA가 아니라 "가장 최근 관측치를 EMA로 스무딩"한 값 하나(`HealHpRatio`)만 유지.

### 2. 관측 훅이 실제로 연결된 위치

초안은 "몬스터 공격 명중/빗나감"을 `UBTTask_Attack` 종료 시점의 플래그로 처리하려 했으나, 실제로는 **몬스터의 공격 판정이 일어나는 지점(`AWeapon::WepaonBeginOverlap`)에서 바로 기록**하는 쪽이 더 정확하고 단순해서 이 방식으로 구현했다:

| 이벤트 | 실제 훅 위치 | 기록 |
|---|---|---|
| 몬스터 공격 명중/회피(무적프레임) | `AWeapon::WepaonBeginOverlap` — `TakeDamage` 반환값이 0보다 크면 명중, 0이면(플레이어 무적 중) 회피 | `RecordAttackResult(MonsterID, CurrentPatternIndex, fDamage > 0.f)` |
| 플레이어 회피 | `APy_Grey::OnPlayerDodge` (6단계) — 서브시스템이 `Initialize()`에서 직접 구독 | `RecordPlayerDodge()` |
| 포션 사용 | `Py_Grey::UsePotionItem` (회복 적용 직전, `GetHpRatio()`로 조회) | `RecordPlayerPotionUse(HpRatio)` |
| 교전 거리 | `UBTService_Detect::TickNode` — 타겟 탐지 성공 시 `FVector::Dist` | `RecordEngageDistance(dist)` |

`AWeapon`은 부착 부모(`GetAttachParentActor()`)를 `AMonster`로 캐스팅해 `MonsterID`/`CurrentPatternIndex`를 얻는다 — 몬스터 서브클래스마다 스폰 코드를 건드릴 필요가 없다. **단, 이 방식은 고블린처럼 `AWeapon` 액터 + `mapWeapon`을 쓰는 몬스터에만 적용된다.** 라바/나바쉬는 자체 `UBoxComponent`로 직접 타격 판정을 하므로(별도 `WepaonBeginOverlap` 구현) 이번 단계에서는 패턴 학습 훅이 연결되지 않았다 — `AttackPatterns`를 비워두면 기존과 동일하게 동작하므로 회귀는 없다. 라바/나바쉬까지 학습을 확장하려면 각 클래스의 `WepaonBeginOverlap`에 동일한 `RecordAttackResult` 호출을 추가해야 한다(향후 작업).

`AMonster`에는 초안이 제안한 `bCurrentAttackHit` 플래그는 두지 않았다 — 명중 여부가 `TakeDamage` 반환값에서 바로 나오므로 불필요했다. `CurrentPatternIndex`만 유지한다(공격 시작 시 `UBTTask_Attack`이 블랙보드 `SelectedPattern`을 읽어 설정).

### 3. 몬스터 공격 패턴 다양화

- `AMonster::AttackPatterns` (`TArray<UAnimMontage*>`) 추가. 비어 있으면 `UBTTask_Attack::AttackMontage`(기존 단일 몽타주)로 폴백 — 다른 몬스터 회귀 없음.
- `UBTTask_Attack::ExecuteTask`가 `AttackPatterns.Num() > 0`이면 블랙보드 `SelectedPattern`(Int)을 읽어 `pMonster->CurrentPatternIndex`에 클램프해 저장하고, 그 인덱스의 몽타주를 재생한다.

### 4. `UBTService_PatternAnalysis : UBTService` (신규)

몬스터 BT에 붙는 서비스. 0.5초 간격으로:

1. `SelectAttackPattern` 호출 → 블랙보드 `SelectedPattern`(Int) 기록 (패턴이 없으면 0을 기록하지만 `BTTask_Attack`이 애초에 그 값을 쓰지 않음).
2. 회피율(`GetDodgeRate`)이 `DodgeRateThreshold`(기본 0.5)를 넘는 만큼 `FeintChance`(Float, 최대 `MaxFeintChance`=0.6)를 선형으로 올려 기록.
3. 평균 교전 거리(`GetAvgEngageDistance`)가 `LongRangeThreshold`(기본 600)를 넘거나, 플레이어 현재 HP 비율이 평소 회복 타이밍(`GetHealHpRatio`) ± `HealRatioTolerance`(기본 0.1) 안에 있으면 `Aggression`(Float, 최대 `MaxAggression`=1.0)을 기록.
4. `bShowDebugStats`가 true면 `PrintViewport`로 현재 학습 상태(회피율/평균거리/패턴 가중치) 출력 — 발표 시연용.

임계값/배율은 전부 서비스 인스턴스의 `UPROPERTY(EditAnywhere)`이므로 BT 에디터에서 몬스터별로 조정 가능.

### 5. 페인트(속임수) 실행 — `UBTTask_Attack`

`ExecuteTask`에서 블랙보드 `FeintChance` 확률로 당첨되면 즉시 몽타주를 재생하지 않고 `bIsFeinting=true` 상태로 `InProgress`를 반환한다. `TickTask`에서 `FeintTimeRemaining`(0.3~0.6초 랜덤)을 줄이다가 0 이하가 되면 그제서야 실제 공격 몽타주를 재생한다(공용 처리 함수 `StartAttackMontage`로 즉시 실행 경로와 통합).

## 초안과의 차이 요약 (2026-07-17, 실제 코드 대조 후)

초안(2026-07-08 작성) 대비 큰 설계 불일치는 없었다 — 이 단계는 05단계 때와 달리 아직 구현되지 않은 새 기능이라 기존 코드와 어긋날 소지가 적었다. 다만 세부 구현에서:

- 명중/회피 판정을 `UBTTask_Attack` 종료 플래그 대신 `AWeapon::WepaonBeginOverlap`의 `TakeDamage` 반환값에서 직접 처리 (더 정확하고 단순).
- `AMonster::bCurrentAttackHit` 플래그는 만들지 않음 (위 이유로 불필요).
- 학습 훅은 `AWeapon` 액터 방식을 쓰는 몬스터(고블린)에만 연결됨. 라바/나바쉬는 자체 박스 콜리전이라 미연결 — `AttackPatterns` 비워두면 회귀 없음.
- `ACharBase::GetHpRatio()` public 헬퍼를 새로 추가함 (회복 습관 판정 및 향후 8단계 보스 HP 바에서도 재사용 가능).

## 에디터 작업 (사용자)

1. 고블린 BT(`BT_Monster`의 `AI.Subtree.Combat` 서브트리 등 실제 공격 브랜치)에 `UBTService_PatternAnalysis` 부착.
2. 블랙보드(`BB_Monster`)에 키 3개 추가: `SelectedPattern`(Int), `FeintChance`(Float), `Aggression`(Float).
3. 고블린 BP(`BP_Mon_Goblin` 등)의 `AttackPatterns` 배열에 공격 몽타주 2~3개 연결 (기존 콤보 몽타주 분할 재사용 가능).
4. (선택) `Aggression` 값에 따라 이동 속도/`UBTTask_Dash` 우선순위를 바꾸고 싶다면 BT 그래프에서 `Aggression` 블랙보드 키를 조건으로 하는 분기를 추가 — C++ 쪽은 값만 채워 두고 실제 분기는 BT 에디터 작업.
5. 시연용으로 `UBTService_PatternAnalysis`의 `bShowDebugStats`를 true로 켜서 화면에 학습 상태 출력 확인.

## 완료 기준 (PIE 테스트)

- [ ] 같은 고블린과 반복 교전 시 공격 패턴(몽타주)이 다양하게 나온다.
- [ ] 회피만 반복하는 플레이 스타일로 몇 번 싸우면, 몬스터가 공격 타이밍을 늦추는 페인트를 섞기 시작한다 (`bShowDebugStats`로 `FeintChance` 상승 확인).
- [ ] 멀리서 도망 다니면 몬스터 추격/공격 판단이 공격적으로 변한다 (`Aggression` 상승 확인).
- [ ] 몬스터가 리스폰돼도 학습 상태가 유지된다 (서브시스템 수명 확인).
- [ ] `AttackPatterns`를 설정하지 않은 몬스터(라바 등)는 기존과 동일하게 동작한다 (회귀 없음).
