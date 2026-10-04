# 18. 몬스터 전리품 / 고블린 토벌 보상

> 골렘·새·애벌레가 전리품을 떨어뜨리고, 그 전리품을 상점 판매 탭에서 팔 수 있다. 고블린 4마리 처치 퀘스트(`고블린 토벌`)를 끝내면 **고블린 소드**를 받는다.
> `DT_Item`, 몬스터 블루프린트, `BP_Hans`(퀘스트 체인)는 **바이너리라서 하나도 건드리지 않았다.** 아이템 정의·아이콘·드롭 표·보상 보충을 전부 C++가 맡는다.

## 1. 한눈에 보기

| 몬스터 | 전리품 (ItemID) | 이름 | 드롭 확률 | 개수 | 판매가 |
|--------|-----------------|------|-----------|------|--------|
| 골렘 `AMon_Golem` | `Loot_GolemCore` | 골렘의 핵 | 80% | 1 | 150 G |
| 새 `AMon_Bird` | `Loot_BirdFeather` | 새의 깃털 | 60% | 1~2 | 35 G |
| 애벌레 `AMon_Larvae` | `Loot_LarvaeSlime` | 애벌레 점액 | 60% | 1 | 25 G |

- 몬스터가 죽으면 바닥에 아이콘이 떠 있는 전리품이 놓이고, 가까이 가서 `E`로 줍는다 (기존 아이템 습득과 같은 방식).
- 인벤토리(`I`)에서 이름·종류(전리품)·설명·판매가가 보이고, 상점 NPC의 **판매 탭**에 자동으로 나온다 (판매가가 있는 아이템만 나오는 기존 규칙 그대로).
- 아이콘 이미지는 텍스처 에셋이 아니라 **코드로 그린 그림**이다 (`IconPainter`). 기본 검/방패 아이콘과 같은 방식.
- 가방(20칸)이 가득 차면 못 줍고 소리만 나며, 전리품은 그 자리에 남아 있다 (자리를 비운 뒤 다시 `E`).

### 고블린 토벌 보상

`BP_Hans`의 두 번째 퀘스트(`고블린 토벌`: 고블린 `Goblin` 4마리 처치)의 블루프린트 보상은 경험치 200 + 골드 3000 + `HpPotion` ×1이다. `UQuestComponent::ResolveRewardItems`가 **처치(Kill) 퀘스트의 대상이 `Goblin`이면 블루프린트의 보상 아이템에 고블린 소드 ×1을 더해서** 돌려준다 (블루프린트가 이미 소드를 주고 있으면 중복해서 주지 않는다).

- 처음에는 "보상 아이템이 비어 있을 때만 소드를 채운다"로 만들었는데, 실제 블루프린트에는 `HpPotion`이 들어 있어서 소드가 한 번도 지급되지 않았다 (플레이 로그로 확인). 그래서 덮어쓰기/비어 있을 때만 채우기가 아니라 **항상 더하기**로 바꿨다.
- 보상 지급(`GrantReward`)과 퀘스트 수락창의 보상 줄(`UUI_Quest::BuildRewardText`)이 같은 함수를 써서 화면에 보이는 것과 실제로 받는 것이 어긋나지 않는다. 수락창에는 `경험치 +200 · 골드 +3000 · 체력 포션 x1 · 고블린 소드 x1`으로 나온다.

## 2. 코드 구성

```
Source/Capstone/
  IconPainter.h/.cpp     (신규) 다각형 여러 장으로 아이콘을 그리는 공용 도구 (안티에일리어싱, 텍스처로 렌더)
  LootData.h/.cpp        (신규) 코드로 정의한 전리품 3종(이름/설명/판매가/스택) + 그 아이콘
  EquipmentData.cpp      아이콘 그리기를 IconPainter로 옮김. GetGeneratedIcon은 장비가 아니면 LootDefs 아이콘을 돌려준다
  ItemData.h             FItemData::Description 추가 (인벤토리 설명 칸에 나옴)
  InventoryComponent.cpp GetItemData: 표에 없는 전리품도 LootData 정의로 행을 만들어 준다 (SyntheticItems 캐시)
  ItemBase.h/.cpp        ItemCount, 아이콘 스프라이트, SpawnLoot(), 가방 가득 참 검사
  Monster.h/.cpp         FMonsterDrop / DropTable / DropLoot() — 죽을 때 항목마다 따로 굴린다
  Mon_Golem/Bird/Larvae  생성자에서 DropTable 기본값
  QuestComponent.*       ResolveRewardItems() (보상 아이템 보충)
  UI_Quest.cpp           보상 줄이 ResolveRewardItems를 쓰고, 아이템 ID 대신 이름을 보여준다
  UI_InventorySlot.cpp   설명 칸에 Description 줄
  Py_Grey.cpp            줍기에 실패하면(가방 가득) CurrItem을 유지
```

- 아이콘을 찾는 세 곳(`UI_InventorySlot`, `UI_PanelParts.h`, `EquipmentComponent`)은 그대로 `EquipDefs::GetGeneratedIcon(ItemID)` 하나만 부른다. DT에 아이콘이 없으면 장비 → 전리품 순으로 코드 아이콘을 찾는다.
- `DT_Item`에 같은 ItemID의 행을 만들면 그 행이 우선한다(이름/아이콘/가격을 에디터에서 바꾸고 싶을 때).
- 기존 방식(블루프린트 픽업을 `DropItemClasses`에 꽂기)은 그대로 동작하고, `DropTable`과 **별개로** 굴린다. 애벌레 BP에 `Loot_LarvaeShell` 픽업이 이미 연결돼 있다면 껍질과 점액이 둘 다 나올 수 있다.

## 3. 조정하는 법

- 확률/개수: 몬스터 클래스 생성자의 `DropTable.Emplace(ItemID, 확률, 최소, 최대)`, 또는 몬스터 블루프린트의 `Drop Items > Drop Table`에서 덮어쓴다.
- 판매가/이름/설명: `LootData.cpp`의 `LootDefinitions()`.
- 아이콘 모양: `LootData.cpp`의 `Build*Shapes` (좌표는 아이콘 한가운데가 (0,0), 가장자리 ±0.5, y가 위쪽).
- 새 전리품 추가: `LootData.h`에 ID 선언 → `LootData.cpp`에 정의/아이콘 → 몬스터 생성자에 `DropTable.Emplace`.
- 고블린 소드 보상 규칙: `QuestComponent.cpp`의 `ResolveRewardItems`.

## 4. 에디터 작업 (사용자)

없음.

## 5. 알아둘 점

- 퀘스트 보상 아이템은 가방이 가득 차 있어도 그대로 들어간다(기존 `AddItem` 동작). 보상을 받기 전에 자리를 비워 두는 편이 좋다.
- 고블린 소드는 `DT_Item`의 기존 행(이름/아이콘/판매가)과 `EquipmentData.cpp`의 장비 수치(공격력 +8)를 쓴다.
- 이미 퀘스트를 끝낸 세이브/진행 상태에는 소급 지급되지 않는다 (완료 시점에 지급되므로).

## 6. 검증한 내용

에디터를 닫은 상태에서 게임(`Lv_Village`)을 직접 실행해 임시 테스트 코드로 확인했다 (테스트 코드는 삭제함).

- 골렘/새/애벌레를 스폰해 죽이면 전리품 3종이 각각 떨어진다 (새는 깃털 2개). 바닥 위에 놓이고 아이콘 스프라이트가 보인다.
- 줍기(`PickupItem`) 후 인벤토리에 `Loot_GolemCore x1`, `Loot_BirdFeather x2`, `Loot_LarvaeSlime x1`이 들어가고, 인벤토리 창에 세 아이콘(보라색 전리품 테두리)과 수량이 나온다.
- 상점 NPC 앞에서 `E` → 판매 탭에 세 전리품이 이름/아이콘/판매가(150/35/25 G)/보유 수와 함께 나온다.
- `고블린 토벌`과 같은 데이터(Kill / `Goblin` / 4 / 경험치 200 / 골드 3000)를 수락시키고 `OnMonsterKilled(Goblin)`을 4번 보내면 완료되면서 골드 +3000, 경험치 +200, `GoblinSword x1`이 인벤토리에 들어간다.
- 퀘스트 수락창 보상 줄: `경험치 +200 · 골드 +3,000 · 고블린 소드 x1`.
- **확인하지 못한 것**: 실제 `E` 키로 줍는 동작(`PickupItem` 직접 호출만 확인), 가방이 가득 찼을 때 줍기 거부, 실제 몬스터 블루프린트(BP_Golem 등)가 죽을 때의 드롭(C++ 클래스를 스폰해 확인), 실제 `BP_Hans`를 통한 퀘스트 수락/완료 흐름, 소리.
- `DT_Item`의 `GoblinSword` 아이콘(`Goblin_Sword_Image`)은 인벤토리/상점에서 회색 바탕에 가는 선만 보이는 그림이다. 코드 아이콘이 아니라 기존 에셋이라 건드리지 않았다.
