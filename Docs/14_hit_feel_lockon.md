# 14단계: 타격감 강화 & 락온

## 목표

6단계에서 만든 타격감(히트스톱, 카메라 셰이크, 진동, 타격 이펙트, 데미지 숫자)은 "때린 순간의 화면 연출"에 치우쳐 있었다.
**맞은 몬스터가 실제로 반응하고, 공격이 잘 맞도록 도와주고, 원하는 적에게 시점을 고정할 수 있게** 만든다.
에디터 작업 없이 C++만으로 동작한다 (마커도 텍스처 없이 코드로 그린다).

## 기존에 있던 것 / 이번에 추가한 것

| 구분 | 내용 |
| --- | --- |
| 기존 (6단계) | 히트스톱, 콤보에 비례하는 카메라 셰이크(`CS_HitWeak/Strong`), 진동, 타격 이펙트+사운드, 몬스터 피격 플래시, 데미지 숫자, 스킬 C 넉백 |
| **락온** | 대상 고정 카메라, 대상 전환, 자동 해제/이어받기, 화면 마커, 락온 대상 HP 바 항상 표시 |
| **일반 공격 넉백** | 일반 공격은 몬스터를 전혀 밀지 못했다 → 타마다 살짝, **마무리 타(4타)는 크게** 밀어낸다 |
| **메시 반동** | 맞은 방향으로 몬스터 메시가 튕겼다 돌아온다. 피격 몽타주가 없는(슈퍼아머) **보스도 맞는 느낌**이 난다 |
| **카메라 펀치** | 명중 순간 FOV가 좁아지며 타격 방향으로 카메라가 파고들었다가 튕겨 돌아온다 (히트스톱 중에도 실시간으로 흐름) |
| **처치 연출** | 결정타는 히트스톱을 길게 + 강한 셰이크 + 진동 (보스는 자체 사망 슬로모션이 있어 히트스톱은 생략) |
| **공격 어시스트** | 일반 공격 스윙이 시작될 때 대상 쪽으로 파고든다 (락온 대상 > 전방 35° 원뿔 안 가장 가까운 몬스터) |
| 히트스톱 재구성 | 타이머 대신 Tick이 실시간 종료 시각을 보고 되돌린다. 연타해도 가장 늦은 종료 시각만 유지하고, 보스 슬로모션 등 다른 시스템이 바꾼 배속은 덮어쓰지 않는다 |

## 조작

| 키 | 동작 |
| --- | --- |
| 마우스 휠 클릭 / `Tab` / 패드 R3 | 락온 토글 (화면 중앙에 가까운 몬스터, 시야가 벽에 가리지 않은 것만) |
| 마우스 휠 위/아래 | 왼쪽/오른쪽 대상으로 전환 |
| 락온 중 마우스를 옆으로 빠르게 쓸기 | 그쪽 대상으로 전환 (`FlickThreshold`로 감도 조절) |

락온 중에는 마우스 좌우/상하가 카메라를 돌리지 않고, 캐릭터가 항상 대상을 바라본다(A/D는 대상 주위를 도는 스트레이프).
입력은 `Config/DefaultInput.ini`의 `LockOn`, `LockOnSwitchNext`, `LockOnSwitchPrev` 액션으로 추가돼 있다.

락온이 **자동으로 풀리는** 경우: 대상 사망(근처 900cm 안에 다른 몬스터가 있으면 자동으로 이어받음), 2100cm 이상 멀어짐, 벽에 1.5초 이상 가려짐, 인벤토리/상점/대사 열림, 플레이어 사망.

## 구조

- `ULockOnComponent` (`APy_Grey::LockOnComponent`): 후보 탐색(카메라 정면 각도 + 거리 점수, LOS 트레이스), 대상 전환(현재 대상 기준 좌/우 각도), 카메라 추적(`RInterpTo`, 실시간 델타), 마커 위젯 관리. 락온 중에만 틱한다.
- `UUI_LockOnMarker`: 뷰포트 전체를 덮는 투명 위젯. 대상의 `GetLockOnPoint()`를 매 프레임 화면에 투영하고 `NativePaint`로 브래킷 4개를 직접 그린다. 락온 순간 크게 시작해 조여드는 연출 + 맥동.
- `AMonster`: `GetLockOnPoint()`(소켓 또는 액터 위치 + 높이 보정), `SetLockedOn()`(HP 바 강제 표시), `ApplyHitRecoil()`(메시 상대 위치를 밀었다가 지수 감쇠로 복귀).
- `APy_Grey::ApplyHitImpact()`: 일반 공격(`WepaonBeginOverlap`)과 스킬 C(`UpdatePowerSlashHit`)가 함께 쓰는 "맞힌 직후의 반응" 진입점. 넉백 → 메시 반동 → 카메라 펀치 → 처치 연출 순.
- `APy_Grey::ApplyAttackAssist()`: `UPy_GreyAnim::InherAttackStartNotify`(스윙 시작 노티파이)가 **일반 공격 몽타주 재생 중일 때만** 호출한다. 목표 거리를 캐릭터 무브먼트의 실제 마찰/감속 값으로 역산해 `LaunchCharacter` 속도를 정하므로, 정확히 그 거리만큼만 파고든다.

## 튜닝 (전부 `BP_PyGrey` / 몬스터 BP의 Details 패널에 노출됨)

| 위치 | 프로퍼티 | 설명 |
| --- | --- | --- |
| `LockOnComponent` | `MaxLockDistance`, `BreakDistance`, `SearchHalfAngle`, `CloseRangeAnyAngle` | 락온 가능/해제 거리, 탐색 각도 |
| `LockOnComponent` | `CameraInterpSpeed`, `CameraPitchOffset`, `CameraPitchMin/Max` | 카메라 붙는 속도(클수록 빡빡), 대상이 화면에 놓이는 높이, 피치 한계 |
| `LockOnComponent` | `FlickThreshold`, `FlickWindow`, `SwitchCooldown` | 마우스 쓸기 대상 전환 감도 |
| `APy_Grey` `HitFeel\|Impact` | `NormalKnockbackScale`, `FinisherKnockbackScale`, `FinisherComboIndex` | 몬스터 `KnockbackStrength`(기본 300)에 곱해지는 밀어내기 배율, 마무리 타 번호 |
| `APy_Grey` `HitFeel\|Impact` | `HitRecoilDistance`, `FinisherRecoilScale` | 메시 반동 거리(cm) |
| `APy_Grey` `HitFeel\|Impact` | `CameraPunchFOV`, `CameraPunchKick`, `CameraPunchReturnSpeed` | 카메라 펀치 세기와 복귀 속도 |
| `APy_Grey` `HitFeel\|Impact` | `KillHitStopScale`, `KillCameraShakeScale` | 결정타 연출 세기 |
| `APy_Grey` `HitFeel\|AttackAssist` | `AttackAssistRange/HalfAngle/StopGap/MaxTravel` | 어시스트 사거리, 원뿔 각도, 멈추는 거리, 한 번에 파고드는 최대 거리 |
| `AMonster` (몬스터 BP) | `HitRecoilScale`, `HitRecoilReturnSpeed`, `MaxHitRecoil` | 몬스터별 반동 (나르바쉬는 생성자에서 0.35배) |
| `AMonster` (몬스터 BP) | `bCanBeLockedOn`, `LockOnSocketName`, `LockOnHeightOffset`, `LockOnMarkerScale` | 락온 제외, 겨냥 지점(소켓 우선), 마커 크기 (나르바쉬는 1.8배) |

## 에디터 작업 (사용자)

필수 작업은 없다. 원하면 아래를 조정한다.

1. 플레이해 보고 `FlickThreshold`(마우스 쓸기 감도)와 `CameraInterpSpeed`(카메라 붙는 속도)를 취향에 맞게 조정한다.
2. 덩치가 큰 몬스터는 BP에서 `LockOnSocketName`(예: 가슴 소켓)이나 `LockOnHeightOffset`, `LockOnMarkerScale`을 지정한다.
3. 마커를 텍스처로 꾸미고 싶다면 `UUI_LockOnMarker`를 부모로 한 WBP를 만들어 `LockOnComponent`의 `MarkerWidgetClass`에 지정한다 (지정하지 않으면 코드로 그린 기본 마커).
4. 몬스터 머티리얼에 `FlashAmount` 스칼라 파라미터가 없으면 피격 플래시가 안 보인다 (6단계부터의 기존 사항).

## 완료 기준 (PIE 테스트)

- [ ] 마우스 휠 클릭(또는 `Tab`)으로 몬스터에 락온되고, 카메라가 부드럽게 대상을 향해 돌아가며 붉은 마커가 조여든다.
- [ ] 락온 중 대상 몬스터의 HP 바가 (아직 나를 감지하지 못했어도) 보인다.
- [ ] 마우스 휠 또는 옆으로 쓸기로 좌/우 몬스터로 전환된다. 반대편에 몬스터가 없으면 아무 일도 일어나지 않는다.
- [ ] 락온 대상이 죽으면 근처 다른 몬스터로 이어지고, 없으면 마커가 사라진다. 인벤토리를 열면 락온이 풀린다.
- [ ] 일반 공격 시 몬스터가 살짝 밀리고 몸이 튕기며, 4타째는 눈에 띄게 멀리 밀려난다. 나르바쉬는 밀리지 않지만 몸이 튕긴다.
- [ ] 명중 순간 화면이 살짝 줌인됐다 돌아온다. 몬스터를 처치한 타는 히트스톱이 더 길다.
- [ ] 몬스터와 떨어진 채로 공격하면 스윙과 함께 대상 쪽으로 파고들고, 이미 붙어 있으면 밀고 들어가지 않는다.
- [ ] 스킬 A/B 시전 중에는 어시스트가 발동하지 않는다.
