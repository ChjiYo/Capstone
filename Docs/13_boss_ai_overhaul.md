# 13단계: 나르바쉬 보스 AI 개편

## 목표

나르바쉬가 "약해 보이는" 원인을 없애고, 예고 → 회피 → 반격이 살아 있는 **보스다운 전투**로 바꾼다.
에디터 작업 없이 C++만으로 동작하며(파라곤 나르바쉬의 애니메이션/파티클/사운드를 코드에서 직접 로드), 문제가 생기면 `bUseCodeBuiltTree = false`로 기존 `BT_Narbash`로 즉시 되돌릴 수 있다.

## 약해 보였던 원인

| 원인 | 조치 |
| --- | --- |
| 피격마다 `Hit()`가 몽타주를 끊어서 공격이 스턴락에 걸림 | `Hit()`를 virtual로 바꾸고 보스는 슈퍼아머. 대신 **강인도(Poise)**가 바닥나면 스턴 |
| 공격이 3연타/돌진(속도 8000) 위주로 단조로움 | 스킬 6종 + 포효, 스킬 선택 브레인 |
| 예고/이펙트/카메라 연출이 거의 없음 | 바닥 예고 표시, 파라곤 이펙트, 카메라 셰이크, 사운드, 사망 슬로모션 |
| 항상 플레이어를 정확히 따라봄 | 예고 시간에만 추적하고 타격 직전에 방향 고정(회피 가능) |
| 탐지 범위 1000이라 살짝만 멀어져도 타겟을 잃고 굳음 | `MinBossTraceRange = 3000` |

## 스킬

| 스킬 | 사거리(cm) | 핵심 |
| --- | --- | --- |
| Roar | - | 조우 시 포효(충격파로 밀어냄). 2페이즈 전환 포효(무적, 광폭화 오라 활성화) |
| Combo | 0~270 | 기존 3연타 몽타주 + 스윙마다 전진 러시, 3타가 가장 강함 |
| Smash | 0~360 | 앞쪽에 붉은 예고 원 → 내려찍기 광역. 2페이즈는 충격파 링이 이어짐 |
| Charge | 550~1700 | 붉은 직선 예고 → 돌진. 명중 시 큰 피해/넉백. 2페이즈는 60% 확률로 연속 돌진 |
| LeapSlam | 450~1300 | 착지 지점 예고 → 도약 → 착지 광역 + 충격파 링 |
| DrumThrow | 500~2200 | 북채 투척. 2페이즈는 3갈래 |
| Rampage | 0~1400 | **2페이즈 전용 궁극기.** 지면 강타 3연속 + 퍼지는 충격파(점프/대시로 회피) |

강인도가 바닥나면 `StaggerDuration`(2.4초) 동안 스턴하고 받는 피해가 1.3배가 된다(포효/궁극기 중에는 스턴 면역).

## 구조

- `AMon_Narbash`: 스킬 시스템 + 브레인. 스킬은 **타임라인**(스킬 시작 기준 상대 시간에 동작 등록)으로 진행된다.
- `UBTTask_BossSkill` / `UBTTask_BossApproach` / `UBTDecorator_BossSkillReady`: 전용 BT 노드. 판단은 보스가, 노드는 실행만 맡는다.
- 전투 서브트리는 `AMon_Narbash::GetCombatBehaviorTree()`가 **코드로 조립**해서 `AMonsterAIController`가 `AI.Subtree.Combat` 슬롯에 주입한다(`AMonster::GetCombatBehaviorTree` 훅 추가).

```
Selector "Narbash Combat"
 ├─ [BossSkillReady, Lower Priority Abort] Sequence -> BossSkill
 └─ BossApproach   (사거리까지 접근 / 후딜·스턴 중 제자리 대기)
```

- `ABossTelegraph`(바닥 예고), `ABossProjectile`(북채), `UBossCameraShake*`(카메라 셰이크): 에셋 없이 코드로만 구성.
- **7단계 적응형 AI 연동**: 스킬 인덱스(= `ENarbashSkill` 값)를 패턴 인덱스로 써서 잘 맞은 스킬의 가중치를 올린다(`UCombatPatternSubsystem::GetPatternWeight` 추가). 원거리 성향 플레이어에게는 돌진/도약/투척, 회피가 잦은 플레이어에게는 광역기와 공격 시작 지연(페인트)을 늘린다.

## 튜닝

에디터의 `BP_Narbash` 디테일 패널에서 조절한다.

- `Boss|AI`: `SkillConfigs`(스킬별 사거리/쿨다운/가중치/피해 배율), `RecoveryTimeRange`(후딜), `IntroRoarDistance`
- `Boss|Poise`: `MaxPoise`, `StaggerDuration` 등
- `Boss|Skill Tuning`: 내려찍기 타격 시점 비율, 돌진 속도/예고 시간, 도약 체공 시간, 투척 속도 등

## 알려진 사항 / 참고

- 처음 조우할 때 포효 시점, 각 스킬의 타격 타이밍은 애니메이션 길이 비율로 잡았다. 실제 플레이 후 어색하면 `SmashStrikeFraction`, `ThrowReleaseFraction` 등으로 미세 조정한다.
- 보스는 외부 넉백(플레이어 파워슬래시 등)에 밀리지 않는다(`LaunchCharacter` 재정의).
- 기존 `PhaseTransitionMontage`는 더 이상 쓰이지 않는다(포효 스킬이 대체). 2페이즈 광폭화는 파편/에너지 오라 이펙트로 표시한다.
- 검증은 게임을 직접 실행해서 스킬별 스크린샷/로그로 확인했다(자동화 테스트는 없음).
