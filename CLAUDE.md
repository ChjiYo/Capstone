# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 프로젝트 개요

Capstone은 Unreal Engine 5.7 기반 C++ 액션 RPG 프로젝트입니다 (모듈명 `Capstone`, `CAPSTONE_API`). 게임플레이 코드는 `Source/Capstone/`에 있고, 그 외(레벨, 블루프린트, 메시, 애니메이션, 위젯)는 `Content/UE_Asset/` 아래의 바이너리 콘텐츠로 언리얼 에디터에서 직접 편집합니다.

엔진 설치 경로: `C:\Epic_Game\UE_5.7`.

## 빌드 / 실행 명령

이 저장소에는 크로스플랫폼 빌드 스크립트나 테스트 하네스가 없습니다 — Windows 전용 UE 프로젝트이며 UnrealBuildTool(UBT) 또는 에디터/Visual Studio로 빌드합니다.

- **VS 프로젝트 파일 생성**: `Capstone.uproject` 우클릭 → "Generate Visual Studio project files", 또는 엔진 설치 경로의 UBT `GenerateProjectFiles`를 직접 실행.
- **컴파일 (에디터 타겟, Development 구성)**:
  ```
  "C:\Epic_Game\UE_5.7\Engine\Build\BatchFiles\Build.bat" CapstoneEditor Win64 Development -project="C:\Unreal_Script\Capstone\Capstone.uproject"
  ```
- **에디터 실행**: `Capstone.uproject`를 열면 됨 (`.uproject`의 `EngineAssociation`에 따라 UE 5.7이 실행됨). 이미 빌드된 경우 `Binaries/Win64`의 실행 파일을 직접 실행해도 됨.
- 자동화된 테스트는 없음 (`Test` 타겟 없음, CI 설정 없음) — 검증은 에디터에서 수동으로(PIE) 플레이테스트하는 방식.
- 빌드 타겟은 두 개: `Capstone.Target.cs`(Game)와 `CapstoneEditor.Target.cs`(Editor).
- 모듈 의존성 (`Capstone.Build.cs`): Core, CoreUObject, Engine, InputCore, EnhancedInput, ApexDestruction, Niagara, GameplayTags, UMG (public); Slate, SlateCore (private).
- 활성화된 플러그인 (`Capstone.uproject`): ModelingToolsEditorMode (에디터 전용), ApexDestruction, CascadeToNiagaraConverter.

## 작업 시 유의사항

- **게임플레이 로직의 원본은 C++**입니다. 블루프린트(`.uasset`)는 보통 C++ 베이스 클래스를 상속해서 메시/애니메이션/위젯을 연결하고 에디터에 노출된 속성을 조정하는 용도입니다. C++에서 기능이 "빠진" 것처럼 보이면 블루프린트 쪽에 설정되어 있는지 확인하세요 (예: `BP_Goblin`, `BP_PyGrey`, `BP_UI_NPC_Quest`) — 이들은 바이너리라서 이름/클래스로만 참조 가능하고 내용을 직접 읽을 수는 없습니다.
- `.uasset`/`.umap` 파일은 바이너리이므로 텍스트로 보거나 diff를 시도하지 마세요. C++과 블루프린트를 동시에 건드리는 작업이라면 `.uasset` 쪽 변경은 사용자가 에디터에서 직접 해야 하며, Claude는 C++ 쪽만 변경할 수 있습니다.
- 일부 기존 헤더의 한글 주석(EUC-KR/CP949로 작성됨)이 UTF-8로 읽으면 깨져 보일 수 있습니다. 이는 기존부터 있던 문제이므로 관련 없는 작업 중에 "고치지" 말고, 새로 작성하는 한글 주석은 UTF-8로 정상 작성하세요.
- **한글이 들어가는 `.h`/`.cpp` 파일은 항상 UTF-8 BOM을 붙여서 저장하세요** (파일 맨 앞에 U+FEFF). UBT는 `/utf-8` 컴파일러 플래그를 항상 강제하므로 BOM 유무는 컴파일 결과에 영향이 없지만, Visual Studio는 BOM이 없으면 인코딩을 시스템 로케일(한글 Windows면 CP949)로 잘못 추측해서 에디터에서 깨져 보이고, 그 상태로 저장하면 실제로 깨지는 문제가 있었습니다. 새로 만드는 파일은 물론, 한글 주석/문자열을 새로 추가하기 위해 기존 파일을 수정할 때도 그 파일에 BOM이 없다면 함께 붙여주세요. (한글이 전혀 없는 파일에는 BOM을 붙일 필요 없음.)
- `GameInfo.h`는 프로젝트 전역에서 쓰는 잡다한 include 모음입니다 (로깅 매크로 `LOG(...)`, `PrintViewport`, `FQuestData`/`FQuestReward` 구조체, `EDir` 등)이고 거의 모든 다른 헤더에서 include되므로, 이 프로젝트의 공용/PCH에 가장 가까운 파일로 취급하면 됩니다.

## 아키텍처

### 캐릭터 계층 구조

```
ACharacter (엔진)
 └─ ACharBase          — 공용 스탯(FINFO: hp/attack), EWEAPON 슬롯별 무기 맵, TakeDamage
     ├─ AMonster        — AI로 제어되는 적 베이스; 퀘스트 추적용 MonsterID, HP 바 위젯 컴포넌트,
     │   │                스폰포인트 역참조, 공격/피격 델리게이트
     │   ├─ AMon_Goblin  (+ AMon_GoblinAnim)
     │   ├─ AMon_Larvae  (+ AMon_LarvaeAnim)
     │   └─ AMon_Narbash (+ AMon_NarbashAnim)
     ├─ ANPC_Base        — 상호작용 가능한 NPC 베이스 (오버랩 스피어 → Interact BlueprintNativeEvent)
     │   └─ ANPC_Quest    — FQuestData를 보유, 상호작용 시 UUI_Quest 표시, UQuestComponent를 통해 보상 지급
     └─ APy_Grey (+ ICombatInterface) — 플레이어 캐릭터; UInventoryComponent, UQuestComponent,
                                         HUD 위젯(HP 바, 인벤토리, 퀘스트 트래커), 대시 몽타주, 무기 박스 오버랩을 보유
```

애니메이션 인스턴스도 동일한 계층 구조를 따릅니다: `UCharBaseAnim`(공통 공격 콤보/피격/사망 노티파이 처리를 `Inher*Notify()` 가상 훅으로 제공) → `UMonsterAnim` → `UMon_GoblinAnim` 등, 그리고 다른 갈래로 `UPy_GreyAnim`, `UNPC_BaseAnim`/`UNPC_QuestAnim`이 있습니다.

### 전투 & 데미지

- `ICombatInterface`는 최소한의 UInterface입니다 (현재는 `SetExplosiveBuffState`만 있음), `APy_Grey`가 구현.
- 근접 공격: `AWeapon`(그리고 `AMon_Goblin::WeaponBox` 같은 몬스터별 무기 박스)이 `UBoxComponent` 오버랩 이벤트로 `TakeDamage`를 호출.
- 원거리/광역: `AEffectDestroy`는 모든 VFX/SFX 기반 스킬 이펙트의 베이스 클래스(나이아가라+사운드를 스폰하고 `fLifeTime` 후 자동 소멸). `AEffect_Projectile`은 `UProjectileMovementComponent`와 충돌 스피어를 추가해 날아가는 투사체를 구현하고, `AEffect_Dot`은 스피어에 겹친 액터에게 타이머로 지속 피해를 줍니다. `UAnimNotify_SpawnSkillEffect`는 몽타주 재생 중 설정 가능한 대상(플레이어/시전자/소켓/무기 소켓)에 `AEffectDestroy` 서브클래스를 스폰하는 애니메이션 노티파이 훅입니다.

### 몬스터 AI (비헤이비어 트리)

- `AMonsterAIController`가 몬스터를 빙의해서 `UBehaviorTree`/블랙보드를 구동합니다.
- 커스텀 BT 노드: `UBTService_Detect`(감지 틱), `UBTTask_TargetMove`, `UBTTask_Dash`, `UBTTask_Attack`(몽타주 기반 공격, abort 처리 포함), `UBTDecorator_AttackDist`(최소/최대 사거리 조건), `UBTDecorator_CheckHit`(피격 리액션 조건).
- `ASpawnPoint`가 몬스터 (재)스폰을 담당하고(`SpawnType`, `SpawnTime`, 무한/유한 모드), 스폰된 `AMonster::pSpawnPoint`에서 역참조됩니다.

### 퀘스트 시스템

- `FQuestData`/`FQuestReward`(`GameInfo.h`에 정의)는 퀘스트 하나를 표현합니다: id/제목/설명, 보상 아이템+수량, 그리고 선택적인 처치 목표(`KillTargetID`가 `AMonster::MonsterID`와 매칭, `KillTargetCount`, 런타임 진행도 `CurrentKillCount`).
- `AMonster::OnMonsterKilled`는 몬스터가 죽을 때마다 방송되는 static 멀티캐스트 델리게이트입니다.
- `UQuestComponent`(`APy_Grey`가 보유)가 `OnMonsterKilled`를 구독해서 현재 활성 퀘스트 하나의 진행도를 추적하고, UI 갱신용으로 `OnQuestTrackerChanged`(블루프린트에서 바인딩 가능)를 방송합니다.
- `ANPC_Quest`가 퀘스트 정의를 보유하고, 상호작용 시 `UUI_Quest`(수락/거절 다이얼로그)를 표시하며, 수락 시 플레이어 인벤토리를 통해 보상을 지급합니다.
- UI는 두 가지로 나뉩니다: `UUI_Quest`는 NPC 다이얼로그(수락/거절), `UUI_QuestTracker`는 항상 표시되는 HUD 트래커 위젯(`WBP_QuestTracker`) — 둘 다 `BindWidgetOptional`을 사용해서 짝을 이루는 블루프린트 위젯에 일부 요소가 없어도 컴파일이 깨지지 않습니다.

### 인벤토리 & 아이템

- `AItemBase`는 월드에 배치되는 픽업 오브젝트입니다(오버랩 스피어 + 메시). `PickupItem`이 `ItemID`를 습득자의 `UInventoryComponent`로 옮깁니다.
- `UInventoryComponent::Items`는 아이템 ID들의 단순한 `TArray<FName>`입니다 (`FQuestReward::RewardItemID`와 동일한 식별자 공간을 공유).
- `UUI_Inventory`/`UUI_InventorySlot`이 인벤토리 위젯을 렌더링하고, `APy_Grey::ToggleInventory`가 이를 열고 닫습니다.

### 포탈 / 레벨 전환

`APortalPoint`가 플레이어와 겹치면 `TargetLevelName`을 로드합니다 — 이 프로젝트에서 유일한 레벨 간 전환 수단입니다.
