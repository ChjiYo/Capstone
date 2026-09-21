# 15. 사운드 시스템 (효과음 / 환경음)

> 이 문서는 프로젝트에 추가된 **효과음 재생 구조**, **직접 합성한 사운드 목록**, **게임플레이 연결 지점**, **튜닝/재생성 방법**을 정리한다.
> 음악(BGM) 3곡(`Keys_Of_Moon`, `Castles_In_The_Sky`, `End_This`)과 기존 애니메이션 노티파이에 연결돼 있던 소리는 그대로 두었다.
> 합성음은 **어두운 중세 판타지 톤**으로 맞췄다 (아래 "사운드 톤" 참고).

## 0. 사운드 톤 (스토리 기준)

나르바쉬가 마을을 불태운 뒤의 복수극이므로 밝은 차임/스파클/고음 아르페지오를 쓰지 않는다.

- **음계**: D 단조 (D2 73.4 / A2 110 / D3 146.8 / F3 174.6 / A3 220 Hz), 완전5도·단3도 위주. 불협은 반음 부딪힘으로만 쓴다.
- **음색**: 류트 뜯는 소리(Karplus-Strong), 교회 종(낮은 험 + 단3도 부분음), 호른, 합창 패드, 팀파니, 철/나무/가죽/양피지 질감.
- **고역 처리**: UI/상호작용은 5.5kHz, 나머지는 9kHz 위를 잘라내고 리버브 감쇠도 어둡게(2.2~3kHz) 잡는다.
- **환경음**: 밝은 새소리 대신 까마귀 울음과 낮은 부엉이 울음, 마을/들판 아래에 깔리는 불길한 저역 패드(`Amb_Dread`).
- 기존 BGM은 사용자가 넣은 곡이라 바꾸지 않았다. 곡 분위기가 밝다면 레벨의 AmbientSound 볼륨을 낮추거나 곡을 교체하는 것을 권장한다.

## 1. 구성 한눈에 보기

```
Tools/SFX/synth.py        합성 라이브러리 (오실레이터, 필터, 리버브, 크리처 보이스 ...)
Tools/SFX/make_sfx.py     레시피 78개 -> SoundSource/*.wav
Tools/SFX/import_sfx.ps1  UnrealEditor-Cmd ImportAssets 로 /Game/UE_Asset/Sounds/Gen/ 에 임포트
Tools/SFX/spectro.py      WAV 스펙트로그램 시트 (귀 대신 눈으로 검수)

Source/Capstone/CapstoneSFX.h/.cpp
  ESFX                     효과음 ID 열거형
  UCapstoneAudioSubsystem  재생기 (GameInstance 서브시스템): 에셋 사전 로드, 감쇠, 변형 선택, 최소 간격
  UCapstoneLevelAudio      레벨(맵 이름)별 환경음/음악 (World 서브시스템)
  CapstoneSFX::Play2D / PlayAt / PlayPersistent2D   호출 지점용 한 줄 헬퍼
```

에셋 이름은 `SFX_<분류>_<이름>[_NN]` 이고, `CapstoneSFX.cpp` 의 표(`Table()`)가 ESFX 와 에셋 경로를 1:1로 잇는다.
볼륨/피치 범위/거리 감쇠/최소 재생 간격도 그 표 한 곳에서만 바꾼다.

## 2. 기존 에셋 재사용 vs 새로 합성

| 구분 | 내용 |
|------|------|
| 재사용 (기존) | Paragon Greystone 보이스(피격 신음/사망/점프/스킬 기합), 검 타격음 `SFX_Player_SwordHit_Flesh/Hard`, 고블린·애벌레·벌레 공격음, 애벌레 피격음, `Quest_Complete`, `Narbash_Death`, SoulCave 발소리(`Footstep_Dirt_Cue`), 보스 테마 `End_This` |
| 이미 연결돼 있어 건드리지 않음 | 플레이어 검 휘두르기(Attack_A~D 노티파이), 대시/걷기(Jog_* 노티파이), 새·골렘 공격/피격, 고블린 피격 |
| 새로 합성 (Python) | UI, 상호작용, 아이템/상점/강화, 레벨업/챕터/엔딩/사망 연출음, 플레이어 피격/보호막/착지, 스킬 A·C, 몬스터 경계음/사망음/무거운 발소리, 마법, 보스(경고/붐/페이즈/사망/투척), 환경음 루프 |

## 3. 게임플레이 연결 지점

| 상황 | 소리 | 위치 |
|------|------|------|
| 모든 UMG 버튼 클릭/호버, 비활성 버튼 클릭 | UI_Click / UI_Hover / UI_Deny | Slate 입력 프로세서 (`CapstoneSFX.cpp`) |
| 인벤토리/상점/강화창/퀘스트창 열기·닫기 | UI_Open / UI_Close | `APy_Grey::ToggleInventory`, `ANPC_Shop/Enhance/Quest` |
| NPC 상호작용 범위 진입 / 창 열림 | Interact_Prompt / Interact_Talk | `ANPC_Base` |
| 아이템 범위 진입 / 획득 / 몬스터 드랍 | Interact_Prompt / UI_ItemPickup / UI_ItemDrop | `AItemBase`, `AMonster::TakeDamage` |
| 채집 / 채집물 리스폰 | Interact_Gather / Interact_Respawn | `AGatherableObject` |
| 상점 구매·판매·골드 부족 | UI_Coin / UI_Sell / UI_Deny | `UUI_ShopSlot` |
| 몬스터 처치 골드 | UI_Coin (0.45초 지연) | `UInventoryComponent` |
| 포션 사용 / 사용 불가 아이템 | UI_Potion / UI_Deny | `APy_Grey::UsePotionItem` |
| 무기 강화 성공 / 레벨업 | UI_Enhance / UI_LevelUp | `APy_Grey` |
| 퀘스트 수락 / 진행 / 완료 | UI_QuestAccept / UI_QuestUpdate / UI_QuestComplete | `ANPC_Quest`, `UQuestComponent` |
| 대사 시작 / 다음 줄 / 타자기 블립 | UI_DialogueOpen / UI_DialogueNext / UI_DialogueBlip | `UStoryComponent`, `UUI_Dialogue` |
| 챕터 카드 / 엔딩 / 플레이어 사망 | UI_ChapterTitle / UI_Ending / UI_Death | `UStoryComponent`, `APy_Grey` |
| 락온 / 전환 / 해제 | UI_LockOn / UI_LockSwitch / UI_LockRelease | `ULockOnComponent` |
| 스킬 사용 / 쿨다운 중 / 쿨다운 완료 | Player_Skill* + 보이스 / UI_Deny / UI_SkillReady | `APy_Grey` |
| 플레이어 피격 / 보호막 생성·피격·파괴 | Player_Hurt + 신음 / Player_Shield* | `APy_Grey::TakeDamage`, `ActivateShield` |
| 점프 / 착지 / 체력 30% 이하 심장 박동 | Player_Jump / Player_Land / Player_Heartbeat | `APy_Grey` |
| 플레이어 검이 몬스터 명중 | Player_HitFlesh(갑옷·바위는 Hard) | `APy_Grey::WepaonBeginOverlap`, 스킬 C 판정 |
| 몬스터 경계 / 공격 / 피격 / 사망 / 발소리 | `FMonsterSFXProfile` 에 따름 | `AMonster`, `AMonsterAIController`, `UBTTask_Attack` |
| 마법 투사체 / 지속 피해 | Magic_Cast·Projectile·Impact / Dot_Tick | `AEffect_Projectile`, `AEffect_Dot` |
| 보스 경고 원 / 투척 / 임팩트 붐 / 페이즈 전환 / 사망 | Boss_* | `ABossTelegraph`, `ABossProjectile`, `AMon_Narbash` |
| 포탈 이동 / 조건 미달 | Portal_Enter(레벨 전환 유지) / UI_Deny | `APortalPoint` |

### 몬스터별 소리 구성 (`FMonsterSFXProfile`)

각 몬스터 생성자가 기본값을 채우고 블루프린트에서 덮어쓸 수 있다. 이미 애니메이션에 연결된 소리는 `None` 으로 비워 겹치지 않게 했다.

| 몬스터 | 경계 | 공격 | 피격 | 사망 | 발소리 |
|--------|------|------|------|------|--------|
| 고블린 | 작은 울음 | 기존 Goblin_Attack | (애니 노티파이) | 작은 사망음 | 흙 발소리 |
| 애벌레 | 작은 울음 (높은 피치) | 기존 Larvae_Attack | 기존 Larvae_Hit | 작은 사망음 | 흙 발소리 |
| 벌레 | 작은 울음 (더 높은 피치) | 기존 Bug_Attack | 합성 Hit_Generic | 작은 사망음 | 흙 발소리 |
| 골렘 | 큰 으르렁 | (애니 노티파이) | (애니 노티파이) | 큰 사망음 | 무거운 발소리 |
| 새 | 까마귀식 거친 울음 | (애니 노티파이) | (애니 노티파이) | 낮게 떨어지는 울음 | 없음 |
| 나르바쉬 | 자체 로직 | 자체 로직 | 자체 로직 | `OnBossDeath` | 보스 발소리 |

## 4. 환경음 / 음악 (`UCapstoneLevelAudio`)

| 레벨 | 추가되는 소리 |
|------|---------------|
| Lv_Village | 바람 + 멀리서 타는 불 + 불길한 저역 패드 + 드문 까마귀 울음 (마을은 불탄 뒤라는 설정) |
| Lv_OpenField / Lv_Field / Lv_DefaultField | 바람 + 저역 패드 + 까마귀 울음 |
| Lv_Boss | 동굴 드론 + 보스 테마 `End_This` (이 레벨에는 음악/환경음이 배치돼 있지 않았음) |
| LV_Cave | 없음 (레벨에 이미 물소리/동굴 바람/음악이 배치돼 있음) |

프로필은 `CapstoneSFX.cpp` 의 `FindProfile()` 에 있다. 새 레벨의 소리를 추가하려면 그 배열에 한 줄 추가하면 된다.
보스 테마가 마음에 들지 않으면 `Lv_Boss` 줄의 `ESFX::Music_Boss` 를 `ESFX::None` 으로 바꾼다.

## 5. 콘솔 명령 (게임 중 `~`)

| 명령 | 설명 |
|------|------|
| `Capstone.PlaySFX <이름>` | 소리 하나 재생. 예) `Capstone.PlaySFX UI_LevelUp` |
| `Capstone.PlayAllSFX [간격]` | 모든 소리를 차례로 재생 (기본 1.5초 간격). 밸런스 점검용 |
| `Capstone.ListSFX` | 이름 목록 로그 출력 |
| `Capstone.SFXVolume <0~2>` | 효과음 마스터 볼륨 배율 (음악/환경음 제외) |
| `Capstone.SFXDebug <0/1/2>` | 1: 재생 로그, 2: 클릭 시 위젯 경로까지 |

## 6. 튜닝 / 재생성

- **볼륨/피치/감쇠 조정**: `CapstoneSFX.cpp` 의 `Table()` 한 줄만 고친다 (`Vol`, `Pitch`, `In(거리 등급)`, `Gap(최소 간격)`).
- **소리 자체 수정**: `Tools/SFX/make_sfx.py` 의 해당 레시피를 고친 뒤 아래 순서로 다시 만든다. 에디터는 닫아 둔다.

```
python Tools/SFX/make_sfx.py
powershell -ExecutionPolicy Bypass -File Tools\SFX\import_sfx.ps1
```

- 임포트는 `-replaceexisting` 이라 같은 이름의 `.uasset` 을 덮어쓴다. 새 이름을 추가했다면 `ESFX` 와 `Table()` 에도 추가한다.
- 에셋이 없으면 시작 시 `LogCapstoneSFX` 경고가 한 번 나오고 해당 소리만 조용히 건너뛴다 (크래시 없음).
- 루프 플래그(`bLooping`)는 임포트 커맨드릿이 켜지 못해서 런타임에 `Table()` 의 `.Loop()` 를 보고 켠다.
- 자막: Paragon 보이스 큐는 영어 자막이 딸려 나오므로 모든 소리를 `bSuppressSubtitles` 컴포넌트로 재생한다.

## 7. 알려진 한계

- 합성음이라 실제 녹음보다 질감이 단순하다. 마음에 들지 않는 소리는 같은 이름의 `.wav` 로 교체해 임포트하면 코드 수정 없이 바뀐다.
- 귀로 직접 튜닝하지 못한 상태에서 만든 초기 밸런스다. `Capstone.PlayAllSFX` 로 한 번 훑어보고 `Table()` 의 `Vol` 을 조정하는 것을 권장한다.
- UI 클릭/호버 소리는 Slate 입력 프로세서로 모든 `SButton` 에 붙는다. 마우스 커서가 숨겨진 조작 상태에서는 검사하지 않는다.
