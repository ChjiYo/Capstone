# Capstone RPG — 기획 & 설계 문서

이 폴더는 캡스톤 프로젝트의 **남은 기능 전체에 대한 기획/설계 문서**입니다.
구현은 이 문서를 읽는 별도의 AI 모델(또는 개발자)이 단계별로 진행합니다.

## 최종 목표 (게임 한 줄 요약)

> **잿불의 나바쉬가 루엔 마을을 불태운 뒤**, 살아남은 그레이가 마을에서 퀘스트(채집 → 사냥 → 강화)를 수행하며 성장하고,
> 상점에서 포션을 사고 전리품을 팔며 준비한 뒤, 플레이어의 행동 패턴을 학습하는 몬스터들을 뚫고
> 보스 맵으로 이동해 **나바쉬**를 처치하면 복수를 마치고 엔딩.

### 퀘스트 구성 (총 4개)

| 구분 | 챕터 (9단계) | 퀘스트 | 목표 | 관련 시스템 |
|------|------|--------|------|------------|
| 기본 1 | 제 1장 · 살아남은 사람들 | 약초 채집 | 맵의 채집 오브젝트에서 약초 5개 수집 | 채집 시스템 (2단계) |
| 기본 2 | 제 2장 · 놈들의 앞잡이 | 고블린 토벌 | 고블린 5마리 처치 | 기존 처치 퀘스트 (구현됨) |
| 기본 3 | 제 3장 · 부러진 검 | 무기 강화 | 무기를 1회 강화 | 강화 시스템 (4단계) |
| 메인 | 제 4장 · 잿불의 성문 | 나바쉬 토벌 | 보스 맵에서 보스 처치 → 엔딩 | 보스전 (8단계) |

## 구현 로드맵 (단계별 문서)

각 단계는 **하나의 독립된 작업 단위**이며, 아래 순서대로 구현한다. 화살표는 의존 관계.

```
1. 아이템 데이터 & 인벤토리 개편   ← 모든 단계의 기반
   ├─→ 2. 채집 시스템
   ├─→ 3. 상점 NPC (포션 구매 / 전리품 판매)
   └─→ 4. 강화 시스템
            └─→ 5. 퀘스트 시스템 확장 (퀘스트 4종 체인)
6. 전투 액션 강화 (스킬 / 쿨다운 / 타격감)     ← 1~5와 독립, 병행 가능
   └─→ 7. 적응형 몬스터 AI (행동 패턴 학습)
            └─→ 8. 보스전 & 엔딩              ← 5, 7 완료 후
                     └─→ 9. 스토리 설계        ← 1~8 완료 후, 게임플레이 위에 서사를 얹는 단계
                              └─→ 10. 대본 (대사 확정)
                                       └─→ 11. 대사/연출 시스템 구현 (C++)
                                                └─→ 12. 스토리 UI 제작 (WBP)
```

> 9~12단계는 **기존 게임플레이를 바꾸지 않는다.** 퀘스트 체인 4개 위에 대사·챕터 연출·복수 서사를 얹기만 하며,
> 대사 데이터를 비워두면 8단계까지의 동작이 그대로 유지되도록 설계돼 있다.

| 문서 | 내용 |
|------|------|
| [01_item_inventory.md](01_item_inventory.md) | 아이템 DataTable 도입, 인벤토리 수량(스택) 구조 개편, 포션 사용 |
| [02_gathering.md](02_gathering.md) | 맵 오브젝트 상호작용 채집 (약초 등), 리스폰 |
| [03_shop_npc.md](03_shop_npc.md) | 상점 NPC — 포션 구매, 몬스터 전리품 판매 |
| [04_enhancement.md](04_enhancement.md) | 대장장이 NPC — 전리품+골드로 무기 강화 |
| [05_quest_expansion.md](05_quest_expansion.md) | 퀘스트 타입(처치/채집/강화) 일반화, 퀘스트 체인, 4개 퀘스트 데이터 |
| [06_combat_skill.md](06_combat_skill.md) | 스킬 쿨다운/UI, 히트스톱, 카메라 셰이크, 회피 무적 등 액션 타격감 |
| [07_adaptive_ai.md](07_adaptive_ai.md) | 플레이어 패턴 관측 + ε-greedy 기반 공격 패턴 적응 AI |
| [08_boss_ending.md](08_boss_ending.md) | 보스 페이즈, 보스 HP 바, 보스 맵 포탈 게이트, 엔딩 화면 |
| [09_story_design.md](09_story_design.md) | 복수 서사 설계 — 로그라인, 등장인물, 챕터 6개, 범위(만들지 않는 것) |
| [10_story_script.md](10_story_script.md) | 대본 전문 41줄 + DataTable 임포트용 CSV, 퀘스트/시스템 문구 교체분 |
| [11_dialogue_system.md](11_dialogue_system.md) | 대사 DataTable, 재생기 컴포넌트, 대사창/챕터 카드 위젯, 트리거 액터, 기존 흐름 삽입 |
| [12_story_ui.md](12_story_ui.md) | `WBP_Dialogue` / `WBP_ChapterTitle` / `WBP_Ending` 제작 명세 (에디터 작업) |
| [13_boss_ai_overhaul.md](13_boss_ai_overhaul.md) | 나르바쉬 보스 스킬 6종 + 강인도(Poise), 코드로 조립한 전투 비헤이비어 트리 |
| [14_hit_feel_lockon.md](14_hit_feel_lockon.md) | 락온(대상 고정 카메라/마커), 일반 공격 넉백·메시 반동·카메라 펀치·처치 연출, 공격 어시스트 |

## 현재 코드 기준점 (이미 구현된 것)

구현 모델은 아래 요약만 믿고 시작해도 된다. 상세 구조는 저장소 루트의 `CLAUDE.md` 참고.

- **캐릭터 계층**: `ACharBase`(스탯 `FINFO{iMaxHp,iHp,iAttack}`, `EWEAPON` 슬롯별 `AWeapon` 맵) → `AMonster`(고블린/라바/나바쉬), `ANPC_Base`→`ANPC_Quest`, 플레이어 `APy_Grey`.
- **플레이어 컴포넌트**: `UInventoryComponent`(현재 `TArray<FName> Items` + `Gold`), `UQuestComponent`(활성 퀘스트 1개, `OnQuestTrackerChanged`), `UExperienceComponent`(경험치/레벨, `OnExpChanged`/`OnLevelUp`).
- **전역 이벤트**: `AMonster::OnMonsterKilled` — `static` 멀티캐스트 델리게이트 `(FName MonsterID, int32 ExpReward, int32 GoldReward)`. 퀘스트/경험치/골드 컴포넌트가 구독 중. **신규 전역 이벤트도 이 패턴을 따른다.**
- **퀘스트 데이터**: `FQuestData`/`FQuestReward`는 `GameInfo.h`에 정의. 현재는 처치 목표(`KillTargetID`/`KillTargetCount`)만 지원.
- **상호작용**: `ANPC_Base`가 오버랩 스피어 + `Interact` BlueprintNativeEvent 제공. `AItemBase`는 오버랩 상태에서 E키(`APy_Grey::OnInteractKeyPressed`)로 `PickupItem`.
- **전투**: 플레이어 `WeaponBox` 오버랩 → `TakeDamage`. 스킬 이펙트는 `AEffectDestroy`(베이스) / `AEffect_Projectile` / `AEffect_Dot` + `UAnimNotify_SpawnSkillEffect`. 스킬 입력 `SkillSlotA/B/C` 존재.
- **몬스터 AI**: `AMonsterAIController` + 비헤이비어 트리. 커스텀 노드 `UBTService_Detect`, `UBTTask_TargetMove/Dash/Attack`, `UBTDecorator_AttackDist/CheckHit`. `ASpawnPoint` 리스폰.
- **레벨 이동**: `APortalPoint`가 오버랩 시 `TargetLevelName` 로드.

## 구현 모델을 위한 공통 규칙 (모든 단계에 적용)

**토큰 절약 규칙**
1. 작업할 단계의 문서 1개 + 그 문서의 "읽어야 할 파일" 목록만 읽는다. 다른 단계 문서나 `Content/`는 읽지 않는다.
2. `.uasset`/`.umap`은 바이너리다. 열지도, diff하지도 않는다. 에디터 작업은 각 문서의 "에디터 작업" 섹션에 정리돼 있으며 **사용자가 직접 수행**한다.
3. 기존 코드의 포맷/주석/인코딩을 건드리지 않는다. 특히 `ItemBase.h`, `NPC_Base.h` 등의 깨진 한글 주석(CP949)은 **고치지 말 것**. 새 주석은 UTF-8 한글로 작성.

**코드 규칙**
1. 새 클래스는 `Source/Capstone/` 평면 구조에 추가 (하위 폴더 없음, 기존 관례).
2. 전역 게임 이벤트는 `AMonster::OnMonsterKilled`처럼 **클래스 static 멀티캐스트 델리게이트**로 선언하고, 구독자는 `FDelegateHandle`을 보관했다가 `EndPlay`에서 반드시 해제한다 (기존 `UQuestComponent` 참고).
3. UI 위젯 C++ 클래스는 `BindWidgetOptional`을 사용해 짝 블루프린트 위젯에 요소가 없어도 컴파일이 깨지지 않게 한다 (기존 `UUI_Quest` 관례).
4. 밸런스 수치(가격, 확률, 회복량 등)는 하드코딩하지 말고 `UPROPERTY(EditAnywhere)` 또는 DataTable로 노출한다.
5. 빌드 확인: `"C:\Epic_Game\UE_5.7\Engine\Build\BatchFiles\Build.bat" CapstoneEditor Win64 Development -project="C:\Unreal_Script\Capstone\Capstone.uproject"`
6. 자동화 테스트는 없다. 각 문서의 "완료 기준(PIE 테스트)"을 사용자가 에디터에서 수동 확인한다.
7. 한 단계 = 한 커밋 단위로 마무리한다.

**문서 형식** — 모든 단계 문서는 동일한 섹션 구조를 가진다:
`목표` → `선행 단계` → `읽어야 할 파일` → `설계` → `에디터 작업 (사용자)` → `완료 기준 (PIE 테스트)`
