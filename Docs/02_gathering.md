# 2단계: 채집 시스템

## 목표

맵에 배치된 오브젝트(약초 덤불 등)를 E키로 상호작용하면 재료 아이템(`Herb` 등)을 획득하고, 오브젝트는 일정 시간 후 다시 채집 가능해진다. 채집 이벤트는 전역으로 방송되어 5단계의 **채집 퀘스트** 진행도로 연결된다.

## 선행 단계

1단계 (아이템 DataTable, 인벤토리 스택).

## 읽어야 할 파일

`ItemBase.h/.cpp`, `Py_Grey.h/.cpp` (상호작용 키 처리부 `OnInteractKeyPressed`, `SetInteractableItem`), `Monster.h` (static 델리게이트 선언 패턴 참고용).

## 설계

### `AGatherableObject : AItemBase`

기존 `AItemBase`가 이미 "오버랩 스피어 + E키 픽업 + `PickupItem(Picker)`" 흐름을 제공하므로 **상속으로 재사용**한다. 차이점만 오버라이드:

```cpp
UCLASS()
class CAPSTONE_API AGatherableObject : public AItemBase
{
    // 1회 채집당 지급 수량
    UPROPERTY(EditAnywhere, Category="Gather") int32 GatherCount = 1;
    // 채집 후 재생성까지 시간(초). 0 이하면 리스폰 없음(1회용)
    UPROPERTY(EditAnywhere, Category="Gather") float RespawnTime = 30.f;

    // 채집이 일어날 때마다 전역 방송 — 채집 퀘스트(5단계)가 구독
    // AMonster::OnMonsterKilled과 동일한 static 멀티캐스트 패턴
    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnItemGathered, FName /*ItemID*/, int32 /*Count*/)
    static FOnItemGathered OnItemGathered;
};
```

동작 규칙:

1. **픽업 처리**: `AItemBase::PickupItem`이 자기 자신을 파괴한다면, `AGatherableObject`는 파괴 대신 **비활성화**해야 한다. `PickupItem`을 `virtual`로 바꾸고(부모 수정 최소화) 오버라이드:
   - 습득자의 `UInventoryComponent`에 `AddItem(ItemID, GatherCount)`.
   - `OnItemGathered.Broadcast(ItemID, GatherCount)`.
   - 메시 숨김(`SetVisibility(false)`) + 콜리전 비활성 + `RespawnTime` 타이머 후 원복. `RespawnTime <= 0`이면 `Destroy()`.
2. **상호작용 프롬프트**: 기존 `AItemBase`가 플레이어에게 `SetInteractableItem(this)`를 넘기는 오버랩 흐름을 그대로 사용 (추가 구현 없음). 화면 안내 문구("E - 채집")는 선택 사항.
3. 채집 모션/캐스팅 시간은 **범위에서 제외** (즉시 획득). 필요하면 추후 몽타주 재생만 추가.

### 부모 클래스 수정 최소화 원칙

`AItemBase`에서 바꿔도 되는 것: `PickupItem`을 `virtual`로 선언, 파괴/지급 로직을 자식이 재정의할 수 있게 분리하는 정도. 그 외 기존 동작(일반 픽업 아이템)은 그대로 유지되어야 한다.

## 에디터 작업 (사용자)

1. `AGatherableObject` 상속 BP `BP_Gather_Herb` 생성 — `ItemID = Herb`, 덤불/약초 메시 지정, `GatherCount`/`RespawnTime` 조정.
2. 마을~사냥터 사이 맵에 `BP_Gather_Herb`를 6~10개 배치 (퀘스트 목표 5개보다 넉넉하게).

## 완료 기준 (PIE 테스트)

- [ ] 약초 오브젝트에 접근해 E키를 누르면 인벤토리에 `Herb`가 추가된다.
- [ ] 채집 직후 오브젝트가 사라지고(숨김), `RespawnTime` 뒤에 같은 자리에서 다시 채집 가능하다.
- [ ] 일반 픽업 아이템(`AItemBase` 원본)의 동작이 그대로다 (회귀 없음).
- [ ] 로그로 `OnItemGathered` 방송이 확인된다 (5단계 연결 전 임시 확인 — `LOG` 매크로 사용).
