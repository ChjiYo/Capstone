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
- 보스 `AMon_Narbash`는 BP의 BT 에셋 대신 **코드로 조립한 전투 서브트리**를 씁니다 (`AMonster::GetCombatBehaviorTree()` 훅 → `AI.Subtree.Combat` 슬롯에 주입, `bUseCodeBuiltTree=false`면 기존 `BT_Narbash`로 폴백). 스킬 선택/실행 로직은 보스 클래스(`ENarbashSkill`, 타임라인 기반)에 있고 `UBTTask_BossSkill`/`UBTTask_BossApproach`/`UBTDecorator_BossSkillReady`는 그것을 호출만 합니다. 자세한 내용은 `Docs/13_boss_ai_overhaul.md`.

### 퀘스트 시스템

- `FQuestData`/`FQuestReward`(`GameInfo.h`에 정의)는 퀘스트 하나를 표현합니다: id/제목/설명, 보상 아이템+수량, 그리고 선택적인 처치 목표(`KillTargetID`가 `AMonster::MonsterID`와 매칭, `KillTargetCount`, 런타임 진행도 `CurrentKillCount`).
- `AMonster::OnMonsterKilled`는 몬스터가 죽을 때마다 방송되는 static 멀티캐스트 델리게이트입니다.
- `UQuestComponent`(`APy_Grey`가 보유)가 `OnMonsterKilled`를 구독해서 현재 활성 퀘스트 하나의 진행도를 추적하고, UI 갱신용으로 `OnQuestTrackerChanged`(블루프린트에서 바인딩 가능)를 방송합니다.
- `ANPC_Quest`가 퀘스트 정의를 보유하고, 상호작용 시 `UUI_Quest`(수락/거절 다이얼로그)를 표시하며, 수락 시 플레이어 인벤토리를 통해 보상을 지급합니다.
- UI는 두 가지로 나뉩니다: `UUI_Quest`는 NPC 다이얼로그(수락/거절), `UUI_QuestTracker`는 항상 표시되는 HUD 트래커 위젯(`WBP_QuestTracker`) — 둘 다 `BindWidgetOptional`을 사용해서 짝을 이루는 블루프린트 위젯에 일부 요소가 없어도 컴파일이 깨지지 않습니다.

### 인벤토리 & 아이템

- `AItemBase`는 월드에 배치되는 픽업 오브젝트입니다(오버랩 스피어 + 메시). `PickupItem`이 `ItemID`를 습득자의 `UInventoryComponent`로 옮깁니다.
- `UInventoryComponent::Items`는 아이템 ID들의 단순한 `TArray<FName>`입니다 (`FQuestReward::RewardItemID`와 동일한 식별자 공간을 공유).
- `UUI_Inventory`/`UUI_InventorySlot`이 인벤토리 위젯을 렌더링하고, `APy_Grey::ToggleInventory`가 이를 열고 닫습니다. 두 위젯 모두 `bUseCodeBuiltLayout`(기본 true)이면 WBP 트리를 버리고 `NativeOnInitialized`에서 코드로 조립합니다 (장비창과 같은 `UI_QuestStyle.h` 팔레트: 골드 이중 테두리, 5x4 칸 격자, 아이템 종류별 테두리 색, 마우스를 올리면 아래 설명 칸에 이름/종류/효과 표시, 소지품 수/소지금). `InventorySlotClass`가 비어 있으면 `UUI_InventorySlot` 자체를 씁니다. WBP의 디자이너 화면에는 예전 레이아웃이 그대로 보입니다.

### 장비 (검/방패, 'U' 장비창)

- `UEquipmentComponent`(`APy_Grey`가 보유)가 무기/방패 칸(`EEquipSlot`)의 착탈, 공격력·최대 체력 보너스(`ACharBase::AddAttackPower/AddMaxHp`로 `m_tInfo`에 더하고 뺌), 손에 보이는 모델을 맡습니다. 장착하면 인벤토리에서 빠지고 해제하면 인벤토리로 돌아갑니다.
- 플레이어 메시(Greystone)의 검/방패는 스킨에 구워져 있어서, 기본 검/방패는 본(`sword_*`, `shield_*`)을 `HideBoneByName`으로 숨기고 보이는 것으로 착탈을 표현합니다. 다른 무기(고블린 소드)는 `UStaticMeshComponent`를 `sword_bottom` 본에 붙입니다.
- 장비 정보의 원본은 `FItemData`의 장비 필드(`EquipSlot` 등)이고, `DT_Item`(바이너리)에 값이 없는 행은 `EquipmentData.cpp`의 코드 정의(`EquipDefs`)를 ItemID로 찾아 씁니다. 기본 스탯이 공격력 5/체력 160이고 기본 검(+5)/방패(+40)를 끼고 시작해 합계가 10/200입니다.
- 장비창 `UUI_Equipment`/`UUI_EquipSlot`은 WBP 없이 C++로만 조립합니다 (`UI_QuestStyle.h` 팔레트 재사용). 인벤토리 슬롯 클릭/드래그로 장착, 장비 칸 클릭/인벤토리로 드래그로 해제. 장착 상태는 `ULevelTransitionSubsystem`을 통해 레벨 이동/리스폰 뒤에도 복원됩니다 (보너스는 이미 저장된 스탯에 포함돼 있으므로 다시 더하지 않음). 자세한 내용은 `Docs/16_equipment.md`.

### 상점 / 강화 창

- `UUI_Shop`/`UUI_ShopSlot`(상점)과 `UUI_Enhance`(강화)도 `bUseCodeBuiltLayout`(기본 true)이면 WBP 트리를 버리고 `NativeOnInitialized`에서 코드로 조립합니다 (인벤토리/장비창과 같은 골드 이중 테두리 판). 공용 부품(창 틀+제목줄, 소지금 배지, 탭/버튼 스타일, 아이콘 칸 `FIconBox`, 아이템 종류별 색/요약)은 `UI_PanelParts.h`의 `PanelUI::`에 있고, 색/브러시는 `UI_QuestStyle.h`를 그대로 씁니다.
- 상점은 `[구매][판매]` 탭 + 수량 칩(×1/×10/최대) + 한 줄 목록이고, **줄을 클릭하면 선택한 수량만큼 바로 거래**합니다 (거래 로직은 줄이 아니라 `UUI_Shop::HandleTradeRequested`). 인벤토리 자리는 `UInventoryComponent::GetAddableCount`로 먼저 확인합니다. 인벤토리 변경 방송은 다음 틱에 한 번만 목록을 다시 그립니다 (줄의 클릭 콜백 안에서 그 줄을 지우지 않으려고).
- 강화 창은 지금 무기·`+N → +N+1`·공격력 변화·재료/골드 충족 여부(부족하면 붉게)를 보여주고, 버튼 글자가 못 하는 이유를 알려줍니다. `Enter` 강화.
- 두 창 모두 `Esc`/`X`/NPC 앞에서 `E` 한 번 더로 닫습니다. NPC가 창을 열고 닫을 때는 `APy_Grey::SetNPCPanelOpen(bool)`을 부릅니다 — 입력 잠금/마우스 커서를 `RefreshPanelInputMode`가 인벤토리·장비창과 함께 판정하므로, 상점을 연 채 `I`/`U`를 여닫아도 커서가 풀리지 않습니다. 자세한 내용은 `Docs/17_shop_enhance_ui.md`.

### 사운드

- `CapstoneSFX.h/.cpp`가 효과음 재생을 전부 맡습니다. `ESFX` 열거형 + `Table()`(에셋 경로/볼륨/피치/거리 감쇠/최소 간격)이 한 곳에 있고, 게임플레이 코드는 `CapstoneSFX::Play2D/PlayAt(this, ESFX::...)` 한 줄만 부릅니다. `UCapstoneLevelAudio`가 맵 이름별 환경음/음악을 깝니다.
- `Content/UE_Asset/Sounds/Gen/`의 에셋은 `Tools/SFX/make_sfx.py`가 합성한 WAV(`SoundSource/`)를 `Tools/SFX/import_sfx.ps1`(커맨드릿, 에디터 닫고 실행)로 임포트한 것입니다. 소리를 바꾸려면 `.uasset`이 아니라 레시피를 고치고 다시 생성하세요.
- 새/골렘/고블린 피격 등 일부 소리는 애니메이션 노티파이(BP 쪽)에 이미 연결돼 있어 C++에서 다시 내지 않습니다 (`FMonsterSFXProfile`의 해당 항목이 `None`). 자세한 내용은 `Docs/15_sound_system.md`.

### 포탈 / 레벨 전환

`APortalPoint`가 플레이어와 겹치면 `TargetLevelName`을 로드합니다 — 이 프로젝트에서 유일한 레벨 간 전환 수단입니다.
