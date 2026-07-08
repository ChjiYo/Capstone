# 1단계: 아이템 데이터 & 인벤토리 개편

## 목표

- 아이템을 **DataTable 한 곳**에서 정의한다 (이름, 종류, 가격, 회복량, 아이콘 등).
- 인벤토리를 "ID 나열(`TArray<FName>`)"에서 **"ID + 수량(스택)"** 구조로 바꾼다.
- **포션 사용**(단축키로 HP 회복)을 구현한다.
- 몬스터가 죽을 때 **전리품 아이템을 드롭**하도록 기존 `DropItemClasses`를 실제로 동작시킨다.

이 단계가 끝나면 상점(3단계)·강화(4단계)·채집 퀘스트(5단계)가 모두 이 데이터 위에서 동작한다.

## 선행 단계

없음 (모든 단계의 기반).

## 읽어야 할 파일

`GameInfo.h`, `InventoryComponent.h/.cpp`, `ItemBase.h/.cpp`, `Monster.h/.cpp`, `CharBase.h/.cpp`, `Py_Grey.h/.cpp`, `UI_Inventory.h/.cpp`, `UI_InventorySlot.h/.cpp` (모두 `Source/Capstone/`).

## 설계

### 1. 아이템 정의 — `FItemData` (DataTable 행)

새 파일 `ItemData.h` (헤더 전용):

```cpp
UENUM(BlueprintType)
enum class EItemType : uint8
{
    Weapon,     // 장비 (고블린 소드 등)
    Potion,     // 소비 아이템 (사용 시 HP 회복)
    Material,   // 채집 재료 (약초 등)
    Loot        // 몬스터 전리품 (판매/강화 재료)
};

USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
    GENERATED_BODY()
    // 행 이름(RowName)이 곧 ItemID. 아래는 부가 정보
    FText DisplayName;
    EItemType ItemType = EItemType::Material;
    int32 BuyPrice = 0;          // 상점 구매가 (0이면 비매품)
    int32 SellPrice = 0;         // 상점 판매가 (0이면 판매 불가)
    int32 MaxStack = 99;         // 스택 상한 (장비는 1)
    int32 HealAmount = 0;        // Potion 전용: 사용 시 회복량
    TSoftObjectPtr<UTexture2D> Icon;   // 인벤토리/상점 슬롯 아이콘
};
```

- 각 필드는 `UPROPERTY(EditAnywhere, BlueprintReadWrite)`.
- DataTable 자산(`DT_Items`)은 에디터에서 생성(에디터 작업 참고).
- C++에서 조회할 수 있게 **DataTable 참조를 한 곳에 둔다**: `UInventoryComponent`에 `UPROPERTY(EditDefaultsOnly) UDataTable* ItemDataTable;` + 정적 헬퍼 `const FItemData* GetItemData(FName ItemID)` 제공. (다른 시스템도 이 헬퍼를 통해 조회.)

### 2. 초기 아이템 목록 (DataTable 행 데이터)

| RowName (ItemID) | DisplayName | Type | Buy | Sell | HealAmount | 비고 |
|---|---|---|---|---|---|---|
| `GoblinSword` | 고블린 소드 | Weapon | 0 | 50 | - | 기존 아이템 유지 |
| `HpPotion` | 체력 포션 | Potion | 30 | 10 | 50 | 상점 구매 대상 |
| `Herb` | 약초 | Material | 0 | 5 | - | 채집 퀘스트 대상 |
| `Loot_GoblinEar` | 고블린 귀 | Loot | 0 | 15 | - | 고블린 드롭, 강화 재료 |
| `Loot_LarvaeShell` | 라바 껍질 | Loot | 0 | 20 | - | 라바 드롭 |

(수치는 초안. 에디터에서 조정 가능해야 하므로 코드에 하드코딩 금지.)

### 3. 인벤토리 스택 구조 — `UInventoryComponent` 개편

```cpp
USTRUCT(BlueprintType)
struct FInventorySlot
{
    GENERATED_BODY()
    UPROPERTY(...) FName ItemID;
    UPROPERTY(...) int32 Count = 0;
};
```

`UInventoryComponent` 변경:

- `TArray<FName> Items` → `TArray<FInventorySlot> Items` 로 교체.
- API (전부 `BlueprintCallable`/`BlueprintPure`):
  - `void AddItem(FName ItemID, int32 Count = 1)` — 같은 ID 슬롯이 있으면 수량 증가(MaxStack 준수), 없으면 슬롯 추가.
  - `bool RemoveItem(FName ItemID, int32 Count = 1)` — 부족하면 false 반환 후 아무 것도 안 함.
  - `int32 GetItemCount(FName ItemID) const`
  - `bool TrySpendGold(int32 Amount)` — 부족 시 false. (상점/강화에서 사용. 기존 `AddGold`/`GetGold`는 유지.)
- **변경 알림 델리게이트 추가**: `DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);` → `UPROPERTY(BlueprintAssignable) FOnInventoryChanged OnInventoryChanged;` — Add/Remove/골드 변화 때마다 방송. 인벤토리 UI·상점 UI·채집 퀘스트(5단계)가 구독한다.
- **호출부 마이그레이션** (시그니처 변경 영향 범위): `AItemBase::PickupItem`, `ANPC_Quest`의 보상 지급(`RewardItemID`/`RewardItemCount` — 이제 Count 인자 활용), `UUI_Inventory`/`UUI_InventorySlot`의 목록 렌더링(슬롯에 수량 텍스트 추가).

### 4. 포션 사용

- `ACharBase`에 회복 함수 추가: `void Heal(int32 Amount)` — `m_tInfo.iHp`를 `iMaxHp` 상한으로 증가. (스탯이 `protected FINFO`라 외부에서 직접 못 만지므로 반드시 멤버 함수로.)
- `APy_Grey`에 입력 추가: 포션 사용 키 (기존 축/액션 바인딩 방식과 동일하게 `SetupPlayerInputComponent`에 등록, 키는 `1` 권장).
  - 동작: 인벤토리에서 `EItemType::Potion` 첫 슬롯 검색 → `RemoveItem` 성공 시 `Heal(HealAmount)` + HP 바 갱신 (기존 `PlayerHpBar` 갱신 경로 재사용).
- 인벤토리 위젯에서 슬롯 클릭으로 사용하는 것은 **선택 사항** (단축키만으로 완료 기준 충족).

### 5. 몬스터 전리품 드롭 연결

- `AMonster`에 이미 `TArray<TSubclassOf<AItemBase>> DropItemClasses`가 있다. 사망 처리 시(현재 `OnMonsterKilled` 방송하는 지점) 각 클래스를 몬스터 위치 주변에 `SpawnActor`로 떨어뜨린다.
- 드롭 확률이 필요하면 `UPROPERTY(EditAnywhere) float DropChance = 1.f;` 하나로 단순화 (아이템별 확률 테이블은 과설계 — 하지 않음).
- 고블린 BP에는 `Loot_GoblinEar` 픽업 BP, 라바 BP에는 `Loot_LarvaeShell` 픽업 BP를 연결 (에디터 작업).

## 에디터 작업 (사용자)

1. `FItemData` 기반 DataTable `DT_Items` 생성, 위 표의 행 입력, 아이콘 텍스처 연결.
2. `BP_PyGrey`(플레이어 BP)의 `InventoryComponent`에 `ItemDataTable = DT_Items` 지정.
3. 전리품 픽업 BP 2개 생성 (`AItemBase` 상속, `ItemID` 각각 `Loot_GoblinEar`/`Loot_LarvaeShell`, 메시 지정) 후 고블린/라바 BP의 `DropItemClasses`에 연결.
4. `WBP_InventorySlot`에 수량 표시 TextBlock 추가 (C++ `BindWidgetOptional` 이름과 일치시킬 것).

## 완료 기준 (PIE 테스트)

- [ ] 같은 아이템을 두 번 주우면 인벤토리에 슬롯 1개 + 수량 2로 표시된다.
- [ ] 고블린 처치 시 전리품이 바닥에 드롭되고 주울 수 있다.
- [ ] 피해를 입은 뒤 포션 사용 키를 누르면 HP가 회복되고 포션 수량이 1 감소한다. 포션이 없으면 아무 일도 일어나지 않는다.
- [ ] 기존 기능 회귀 없음: 퀘스트 보상 아이템 지급, 골드 획득, 인벤토리 토글(기존 키)이 여전히 동작한다.
