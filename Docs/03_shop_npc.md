# 3단계: 상점 NPC (포션 구매 / 전리품 판매)

## 목표

마을에 상점 NPC를 배치한다. 상호작용하면 상점 UI가 열리고,

- **구매**: NPC가 파는 아이템(포션 등)을 골드로 구매 → 인벤토리에 추가.
- **판매**: 인벤토리의 판매 가능 아이템(전리품 등)을 골드로 판매 → 인벤토리에서 제거.

## 선행 단계

1단계 (`FItemData`의 `BuyPrice`/`SellPrice`, `TrySpendGold`, `OnInventoryChanged`).

## 읽어야 할 파일

`NPC_Base.h/.cpp`, `NPC_Quest.h/.cpp` (**UI 열기/입력 모드 전환/복구 패턴의 표준 예시** — `Interact_Implementation`, `CurrentInteractor`, `RestoreInputMode`), `InventoryComponent.h`, `UI_Quest.h/.cpp` (위젯 작성 관례), `UI_InventorySlot.h/.cpp`.

## 설계

### `ANPC_Shop : ANPC_Base`

```cpp
UCLASS()
class CAPSTONE_API ANPC_Shop : public ANPC_Base
{
    // 이 상점이 파는 아이템 ID 목록 (DT_Items의 RowName)
    UPROPERTY(EditAnywhere, Category="Shop") TArray<FName> ShopItemIDs;

    UPROPERTY(EditDefaultsOnly, Category="UI") TSubclassOf<class UUI_Shop> ShopWidgetClass;
    UPROPERTY() UUI_Shop* ShopWidget;
    UPROPERTY() AActor* CurrentInteractor;   // NPC_Quest와 동일 패턴

    virtual void Interact_Implementation(AActor* Interactor) override;
    void CloseShop();   // 위젯 제거 + 입력 모드 복구 (NPC_Quest::RestoreInputMode 참고)
};
```

- `Interact_Implementation`: `ANPC_Quest`와 동일하게 위젯 생성 → 뷰포트 추가 → UI 입력 모드 + 마우스 커서 표시. 닫기 버튼/ESC 시 `CloseShop`.

### `UUI_Shop : UUserWidget`

- 구성 (전부 `BindWidgetOptional`):
  - 구매 목록 패널: `ShopItemIDs`를 순회하며 슬롯 생성 (아이콘, 이름, `BuyPrice`, 구매 버튼).
  - 판매 목록 패널: 플레이어 인벤토리에서 `SellPrice > 0`인 슬롯만 나열 (아이콘, 이름, 보유 수량, `SellPrice`, 판매 버튼).
  - 보유 골드 텍스트, 닫기 버튼.
- 슬롯 위젯은 신규 `UUI_ShopSlot` 하나로 구매/판매 겸용 (모드 enum 하나로 구분). 기존 `UUI_InventorySlot` 작성 관례를 따른다.
- 거래 로직 (UI에서 직접 인벤토리 컴포넌트 호출):
  - 구매: `TrySpendGold(BuyPrice)` 성공 시 `AddItem(ItemID, 1)`. 실패(골드 부족) 시 아무 것도 안 함 (실패 사운드/문구는 선택).
  - 판매: `RemoveItem(ItemID, 1)` 성공 시 `AddGold(SellPrice)`.
  - 거래 후 목록/골드 갱신은 `OnInventoryChanged` 구독으로 처리 (직접 새로고침 호출 금지 — 이벤트 하나로 통일).
- 수량 일괄 구매/판매(N개씩)는 **범위 제외**. 1개씩 거래만.

### 골드 수급 확인

몬스터 처치 골드(기존 `GoldReward` → `UInventoryComponent::HandleMonsterKilled`)와 전리품 판매가 골드 수입원이 된다. 별도 작업 없음.

## 에디터 작업 (사용자)

1. `ANPC_Shop` 상속 BP 생성 (`BP_NPC_Shop`) — 메시/애니BP 연결(기존 NPC BP 참고), `ShopItemIDs = [HpPotion]` 설정, `NPCName`/`NPCTypeTitle`(예: "상인") 입력.
2. `WBP_Shop`, `WBP_ShopSlot` 위젯 BP 생성, C++ `BindWidgetOptional` 이름과 위젯 이름 일치시키기. `BP_NPC_Shop`의 `ShopWidgetClass`에 연결.
3. 마을 맵에 `BP_NPC_Shop` 배치.

## 완료 기준 (PIE 테스트)

- [ ] 상점 NPC에게 E키 → 상점 UI가 열리고 마우스로 조작 가능, 닫으면 게임 입력으로 복귀한다.
- [ ] 골드가 충분할 때 포션 구매 → 골드 감소 + 인벤토리에 포션 추가. 골드 부족 시 구매되지 않는다.
- [ ] 전리품(고블린 귀) 판매 → 인벤토리에서 1개 감소 + 골드 증가. 수량이 0이 되면 판매 목록에서 사라진다.
- [ ] 상점 UI가 열린 동안 캐릭터가 움직이지 않는다 (입력 모드 전환 확인).
