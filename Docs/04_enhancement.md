# 4단계: 강화 시스템 (대장장이 NPC)

## 목표

대장장이 NPC와 상호작용해 **전리품 + 골드**를 소모하고 무기를 강화한다. 강화할 때마다 플레이어 공격력이 오르고, 강화 이벤트가 전역으로 방송되어 5단계의 **강화 퀘스트** 진행도로 연결된다.

## 선행 단계

1단계 (인벤토리 스택, `TrySpendGold`), 3단계 (NPC UI 열기/닫기 패턴 재사용).

## 읽어야 할 파일

`CharBase.h/.cpp` (`FINFO m_tInfo` 스탯 구조), `Py_Grey.h/.cpp` (`HandleLevelUp`의 스탯 증가 방식 참고), `NPC_Shop`(3단계 결과물), `InventoryComponent.h`, `Weapon.h`.

## 설계

### 강화 상태는 어디에 두나 — 결정: `APy_Grey`에 보관

`AWeapon`에 두면 무기 교체/리스폰 시 상태 관리가 복잡해진다. 이 게임은 플레이어 1명·주무기 1개이므로 **플레이어가 강화 수치를 소유**하는 단순한 모델을 쓴다.

```cpp
// APy_Grey에 추가
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enhance") int32 WeaponLevel = 0;

// 강화 성공 시 전역 방송 — 강화 퀘스트(5단계)가 구독. static 멀티캐스트 패턴
DECLARE_MULTICAST_DELEGATE_OneParam(FOnWeaponEnhanced, int32 /*NewLevel*/)
static FOnWeaponEnhanced OnWeaponEnhanced;

// 강화 시도. 재료/골드 검사→소모→공격력 증가→방송. 성공 여부 반환
UFUNCTION(BlueprintCallable) bool TryEnhanceWeapon();
```

- 공격력 반영: `ACharBase`에 `void AddAttackPower(int32 Amount)` 추가 (`m_tInfo.iAttack += Amount`). 레벨업 스탯 증가(`HandleLevelUp`)와 같은 계층에서 처리되므로 데미지 경로는 건드릴 것 없음.
- 강화 비용/증가량은 `APy_Grey`에 에디터 노출 프로퍼티로:

```cpp
UPROPERTY(EditAnywhere, Category="Enhance") FName EnhanceMaterialID = "Loot_GoblinEar";
UPROPERTY(EditAnywhere, Category="Enhance") int32 EnhanceMaterialCount = 3;   // 1회당 재료 수
UPROPERTY(EditAnywhere, Category="Enhance") int32 EnhanceGoldCost = 50;       // 1회당 골드
UPROPERTY(EditAnywhere, Category="Enhance") int32 AttackPerEnhance = 10;      // 1회당 공격력 증가
UPROPERTY(EditAnywhere, Category="Enhance") int32 MaxWeaponLevel = 5;
```

- 비용을 레벨별로 올리고 싶으면 `EnhanceGoldCost * (WeaponLevel + 1)` 정도의 단순 배율만 사용 (테이블 과설계 금지).
- **강화 실패 확률은 범위 제외** (항상 성공). 캡스톤 시연에서 실패는 스트레스만 준다. 확률이 필요해지면 `SuccessRate` 프로퍼티 하나만 추가하면 되는 구조로 남겨둔다.

### `ANPC_Enhance : ANPC_Base` + `UUI_Enhance`

- 3단계 `ANPC_Shop`과 완전히 같은 골격 (Interact → 위젯 생성 → UI 입력 모드 → 닫기 시 복구).
- `UUI_Enhance` 구성 (`BindWidgetOptional`):
  - 현재 강화 수치 (`+N`), 현재 공격력, 다음 강화 비용(재료 아이콘·수량, 골드), 보유 재료/골드.
  - [강화] 버튼 → `APy_Grey::TryEnhanceWeapon()` 호출, 결과에 따라 UI 갱신 (`OnInventoryChanged` + `OnWeaponEnhanced` 구독).
  - 재료·골드 부족 또는 `MaxWeaponLevel` 도달 시 버튼 비활성화.
- 강화 성공 연출(파티클/사운드)은 선택 사항 — `AEffectDestroy` 서브클래스 스폰으로 붙일 수 있는 구조만 유지.

## 에디터 작업 (사용자)

1. `ANPC_Enhance` 상속 BP (`BP_NPC_Enhance`) 생성 — 메시/애니BP, `NPCTypeTitle`(예: "대장장이") 설정, 마을 배치.
2. `WBP_Enhance` 위젯 BP 생성 후 `EnhanceWidgetClass`에 연결.
3. 밸런스 확인: 고블린 3~4마리 사냥이면 1회 강화 재료가 모이는 수준인지 드롭/비용 수치 조정.

## 완료 기준 (PIE 테스트)

- [ ] 재료 3개 + 골드 50을 갖고 대장장이에게 강화 → 재료/골드 차감, `WeaponLevel` +1, 공격력 상승(몬스터가 더 빨리 죽는 것으로 확인 가능).
- [ ] 재료 또는 골드 부족 시 강화 버튼이 눌리지 않거나 아무 일도 일어나지 않는다.
- [ ] `MaxWeaponLevel` 도달 시 더 이상 강화되지 않는다.
- [ ] 로그로 `OnWeaponEnhanced` 방송 확인 (5단계 연결 전 임시 확인).
