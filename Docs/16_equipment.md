# 16. 장비 시스템 (장비창 / 착탈 / 손에 드는 무기)

> 'U' 키로 여는 **장비창**, 인벤토리와 장비창 사이의 **장착/해제**, 장비가 주는 **공격력·체력 보너스**, 그리고 장착한 무기가 **캐릭터 손에 실제로 보이는 것**까지의 구현을 정리한다.
> 위젯 블루프린트(`.uasset`)는 하나도 건드리지 않았고, 장비창은 C++로만 조립한다. (사망 화면 `UUI_Death`와 같은 방식)

## 1. 한눈에 보기

| 항목 | 내용 |
|------|------|
| 열기/닫기 | `U` 키 (`Config/DefaultInput.ini`의 `Equipment` 액션), 창 오른쪽 위 `X` 버튼 |
| 장비 칸 | **무기**(오른손 검), **방패**(왼손) — `EEquipSlot` (`ItemData.h`) |
| 표시 | 무기/방패 칸(아이콘·이름·보너스), 현재 **공격력**과 **체력**(기본값 + 장비 보너스 내역) |
| 장착 | 인벤토리의 장비 아이템을 **클릭**하거나 장비창의 알맞은 칸으로 **끌어다 놓기** |
| 해제 | 장비 칸을 **클릭**하거나 **인벤토리 창으로 끌어다 놓기** |
| 손에 드는 무기 | 장착하면 손에 모델이 붙고, 해제하면 사라진다 (기본 검/방패 포함) |
| 저장 | 포탈 이동/사망 리스폰을 거쳐도 장착 상태가 유지된다 |

인벤토리(`I`)와 장비창(`U`)은 **동시에 열어둘 수 있다.** 둘 중 하나라도 열려 있으면 공격/스킬 입력이 잠기고 마우스 커서가 켜지며, 둘 다 닫히면 게임 입력으로 돌아간다 (`APy_Grey::RefreshPanelInputMode`).

## 2. 코드 구성

```
Source/Capstone/
  ItemData.h                 EEquipSlot 열거형 + FItemData의 장비 필드(EquipSlot/AttackBonus/MaxHpBonus/EquipMesh/EquipSocket/EquipOffset)
  EquipmentData.h/.cpp       FEquipInfo(장비 하나의 게임용 정보) + 코드로 정의한 장비 목록(EquipDefs) + 코드로 그린 아이콘
  EquipmentComponent.h/.cpp  UEquipmentComponent: 칸별 장착 상태, 착탈, 스탯 반영, 손에 붙는 모델, 저장/복원
  UI_Equipment.h/.cpp        UUI_Equipment: 장비창 본체 (코드로 위젯 트리 조립)
  UI_EquipSlot.h/.cpp        UUI_EquipSlot: 장비창의 칸 하나 (클릭 해제 / 드롭 장착 / 드래그 시작)
  UI_ItemDrag.h/.cpp         UItemDragDropOperation: 끌고 있는 아이템 정보 + 마우스를 따라다니는 카드
  UI_InventorySlot.*         (수정) 장비 아이템 클릭 장착, 끌기 시작, 툴팁, 아이콘 폴백
  UI_Inventory.*             (수정) 장비 칸을 이 창 위에 놓으면 해제, 닫기 버튼이 두 창의 입력 상태를 함께 정리
  InventoryComponent.*       (수정) 코드 정의 장비도 GetItemData로 조회, 스택 넘침 처리, CanAddItem, 수량 합산
  CharBase.*                 (수정) AddMaxHp() 추가
  LevelTransitionSubsystem.h (수정) 장착 상태 보관 (SetPending/ConsumePendingEquipment)
  Py_Grey.*                  (수정) EquipmentComponent 보유, U 키, 시작 시 기본 장비, 레벨 이동 저장/복원
```

## 3. 장비 데이터는 어디에 정의되나

장비 정보는 두 곳에서 온다. **DataTable 행에 `EquipSlot`이 채워져 있으면 그 행이 원본**이고, 비어 있으면(=`None`) `EquipmentData.cpp`의 코드 정의를 ItemID로 찾아 쓴다.
이렇게 한 이유는 `DT_Item`이 바이너리 에셋이라 Claude가 행을 추가/수정할 수 없기 때문이다. 에디터에서 행을 채우면 코드 정의보다 우선한다.

현재 코드에 정의된 장비 (`EquipmentData.cpp`의 `Definitions()`):

| ItemID | 이름 | 칸 | 공격력 | 최대 체력 | 모습 |
|--------|------|----|-------|----------|------|
| `GreystoneSword` | 그레이스톤 검 | 무기 | +5 | 0 | 스킨에 구워진 기본 검 (본을 보이기/숨기기) |
| `GreystoneShield` | 그레이스톤 방패 | 방패 | 0 | +40 | 스킨에 구워진 기본 방패 |
| `GoblinSword` | 고블린 소드 | 무기 | +8 | 0 | `SM_Weapons/SM_Sword`를 오른손 `sword_bottom` 본에 붙임 (크기 x1.5, X축 35도 기울임) |

- `GoblinSword`는 `DT_Item`에 이미 행(이름/아이콘/판매가)이 있으므로 그쪽 값을 쓰고, 장비 수치와 모델만 코드에서 채운다.
- `GreystoneSword`/`GreystoneShield`는 표에 행이 없다. 인벤토리에 들어가면 `GetItemData`가 코드 정의로 행을 만들어 주고, 아이콘은 `EquipDefs::GetGeneratedIcon`이 코드로 그린 검/방패 그림을 쓴다 (텍스처 에셋 없음).
- **기본 수치를 맞춘 방법**: 플레이어의 기본 스탯을 공격력 10 / 체력 200에서 **5 / 160**으로 낮추고, 기본 검(+5)과 방패(+40)를 끼고 시작하게 했다. 시작 상태의 합계는 예전과 같은 10 / 200이다.

## 4. 장착/해제 규칙 (`UEquipmentComponent`)

- **장착** `EquipFromInventory(ItemID)`: 인벤토리에서 1개 빼서 칸에 넣는다. 그 칸에 이미 낀 장비가 있으면 인벤토리로 돌려보낸다(맞바꿈). 돌려보낼 자리가 없으면 아무것도 바뀌지 않고 실패음이 난다.
- **해제** `Unequip(Slot)`: 장비를 인벤토리로 돌려보낸다. 인벤토리가 가득 차 있으면(20칸) 실패한다. (`UInventoryComponent::MaxSlots`)
- **스탯**: 장착하면 보너스를 `m_tInfo`(공격력/최대 체력)에 더하고 해제하면 뺀다. 무기 강화·레벨업으로 오른 수치는 그대로 유지된다.
- **체력 보너스의 안전장치** (`ACharBase::AddMaxHp`): 다친 상태에서 방패를 껐다 끼는 것으로 공짜 회복을 하지 못하게, 최대 체력이 늘 때는 **원래 풀피였을 때만** 현재 체력도 함께 채운다. 줄어들 때는 현재 체력이 새 최대치를 넘지 않게 자른다. 장비를 바꾸면 HUD 체력바도 즉시 갱신된다.
- **장비가 없는 상태**: 무기를 벗어도 공격 입력을 막지 않는다 (맨손 모션이 아니라 검 모션이 검 없이 재생됨). 타격 판정은 검 본의 소켓(`FX_Sword_*`)을 쓰는데, 본을 숨겨도 소켓 위치는 그대로인 것을 로그로 확인했다. 다만 무기 없이 실제로 몬스터를 때려 보지는 못했다.

## 5. 손에 드는 모습

플레이어 메시(Paragon Greystone)는 검과 방패가 **몸 스킨에 구워져 있고**, 본(`sword_bottom/sword_top`, `shield_inner/shield_outer`)에 스키닝돼 있다.

- **기본 검/방패**: 별도 모델이 없으므로 그 칸의 장비가 기본 장비가 아니게 되면(벗거나 다른 장비를 끼면) 해당 본을 `HideBoneByName`으로 숨겨 모델을 지우고, 다시 끼면 `UnHideBoneByName`으로 보인다. 소켓 위치(`FX_Sword_*`)는 본 숨김의 영향을 받지 않아서 타격 판정과 스킬 C의 이펙트 위치는 그대로다. (테스트로 확인함)
- **다른 무기 (고블린 소드 등)**: `EquipMesh`(정적 메시)를 `EquipSocket` 소켓/본에 `EquipOffset` 변환으로 붙이는 `UStaticMeshComponent`를 런타임에 만든다. 해제하면 파괴한다. 이때 기본 검 본은 숨겨진다.

새 무기를 추가할 때 손 위치/방향은 `EquipOffset`으로 맞춘다. `sword_bottom` 본은 +Z 방향이 칼날 방향이고 손잡이 위치가 본 원점 근처다.
고블린 소드는 칼날이 휘어 있어서, 아무 회전 없이 붙이면 칼날이 앞쪽으로 수평하게 뻗어 어색했다. X축(FRotator의 Roll)으로 35도 눕혀 몸 옆으로 자연스럽게 늘어지게 맞췄다 (정면/측면 스크린샷으로 비교해서 고름).

## 6. 저장 (레벨 이동 / 사망 리스폰)

`OpenLevel`은 폰을 파괴하므로 `ULevelTransitionSubsystem`에 장착 아이템 ID(칸 순서: 무기, 방패)를 맡긴다 (`APy_Grey::SaveStateForLevelTransition`). 새 폰은 `BeginPlay`에서 모습만 복원하고 **보너스는 다시 더하지 않는다** — 이미 저장된 공격력/체력에 들어 있기 때문이다. 저장된 값이 없으면(게임 첫 시작) `EquipDefaults()`로 기본 검/방패를 끼고 보너스를 더한다.

## 7. 인벤토리 쪽에서 함께 바뀐 것

- `GetItemData`가 DataTable에 없는 코드 정의 장비도 행을 만들어 돌려준다 (`SyntheticItems` 캐시).
- `AddItem`: 스택이 가득 차면 넘치는 만큼을 **새 스택**으로 만든다. (예전에는 넘치는 아이템이 사라졌고, 장비처럼 `MaxStack`이 1인 아이템을 두 개 이상 가질 수 없었다.)
- `GetItemCount`/`RemoveItem`: 여러 스택에 나뉜 같은 아이템을 합산/순차 차감한다.
- `CanAddItem(ItemID, Count)`: 자리가 있는지 미리 확인 (장비 해제가 사용).

## 8. 에디터에서 할 일

**필수 작업은 없다.** 입력 매핑은 `DefaultInput.ini`에 이미 넣었고, 장비창은 코드로 만든다.

선택 사항:
1. **장비창 모양을 바꾸고 싶다면**: 위젯 블루프린트를 만들 필요 없이 `UI_Equipment.cpp`의 `BuildTree()`(색/글자 크기/배치)를 고치는 것이 가장 쉽다. 색 팔레트는 퀘스트 UI와 같은 `UI_QuestStyle.h`의 `QuestUI::`를 쓴다.
2. **새 장비 아이템 추가**: `DT_Item`에 행을 추가하고 `Equip Slot`(Weapon/Shield), `Attack Bonus`, `Max Hp Bonus`, `Equip Mesh`, `Equip Socket`, `Equip Offset`을 채운다. 그 행의 ItemID로 `AItemBase` 픽업이나 상점 목록을 만들면 곧바로 장착 가능하다.
3. 기존 `WBP_Inventory`/`WBP_InventorySlot`은 수정할 필요가 없다. 슬롯의 툴팁/클릭/끌기는 C++ 부모 클래스(`UUI_InventorySlot`)에서 처리한다.

## 9. 알아둘 점

- 방패는 지금 **체력 보너스만** 준다. 막기/피해 감소 같은 방어 동작은 없다 (스킬 B의 보호막은 그대로 별개).
- 무기 강화(`WeaponLevel`, 대장장이 NPC)는 "장착한 무기"가 아니라 플레이어 공격력 자체를 올리는 기존 방식 그대로다. 무기를 바꿔도 강화 수치는 남는다.
- 스킬 A(공격력 강화 버프)는 장비 보너스가 포함된 공격력에 배율이 곱해진다. 장비창의 공격력 아래 줄에 강화 중인 실제 값이 함께 표시된다.
- 장비창은 화면 왼쪽(가로 20% 지점)에 놓인다. 인벤토리 창은 WBP에서 위치가 정해져 있으므로, 겹친다면 `UUI_Equipment::PlaceInViewport`의 앵커 값만 조정하면 된다. (`SetPositionInViewport`를 부르면 앵커가 (0,0)으로 되돌아가 창이 화면 밖으로 나가니 쓰지 말 것)
- 기본 검/방패 아이콘은 텍스처 에셋이 없어 코드로 그린 그림(`EquipDefs::GetGeneratedIcon`)이다. 마음에 들지 않으면 `DT_Item`에 `GreystoneSword`/`GreystoneShield` 행을 만들고 `Icon`을 지정하면 그 아이콘이 우선한다.
- 플레이 도중 얻을 수 있는 장비는 기존 `BP_Item_Goblin_Sword` 픽업(ItemID `GoblinSword`)뿐이다. 그 픽업이 어디에 놓여 있는지/몬스터가 떨어뜨리는지는 BP 쪽 설정이라 확인하지 못했다. 다른 장비 아이템은 위 "새 장비 아이템 추가" 절차로 만든다.

## 10. 검증한 내용

에디터를 닫은 상태에서 게임을 직접 실행해(임시 테스트 코드, 삭제함) 확인했다.

- 시작 상태: 기본 검/방패 장착, 공격력 10 / 체력 200 (예전과 동일). 검·방패를 모두 벗으면 5 / 160, 스크린샷에서 검과 방패 모델이 사라짐.
- 고블린 소드 장착: 공격력 13, 손에 고블린 소드가 붙고 기본 검 모델은 숨겨짐. 벗은 그레이스톤 검은 인벤토리로 돌아감.
- 인벤토리 슬롯 클릭 장착 / 장비 칸 클릭 해제 / 인벤토리 → 장비 칸 끌어다 놓기 / 방패 칸 → 인벤토리 끌어다 놓기 (Slate 합성 마우스 이벤트) — 모두 의도대로 동작하고 스탯이 즉시 갱신됨.
- 방패를 벗으면 체력이 200/200에서 160/160으로 잘림.
- `U`/`I` 키 매핑, 두 창을 함께 열고 하나만 닫아도 입력 잠금·커서가 유지되고 둘 다 닫히면 풀림. 닫았다 다시 열어도 내용이 갱신됨.
- 레벨 재로드(`OpenLevel`) 뒤에도 장착 상태, 공격력, 다친 체력(100/160), 인벤토리가 그대로 복원됨.
- **확인하지 못한 것**: 실제 마우스로 하는 드래그의 손맛, 툴팁 표시, 장착한 상태로 공격 애니메이션 중 고블린 소드의 궤적(정지 자세만 확인함), 게임패드 조작.
