#pragma once

#include "GameInfo.h"
#include "EquipmentData.h"
#include "Components/ActorComponent.h"
#include "EquipmentComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentChanged);

// 플레이어가 지금 끼고 있는 장비(무기/방패)와, 그 장비가 주는 공격력/체력 보너스, 손에 보이는 모델을 관리한다.
// 장착/해제는 인벤토리와 아이템을 주고받는 것으로 이루어진다: 장착하면 인벤토리에서 빠지고, 해제하면 인벤토리로 돌아간다.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CAPSTONE_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEquipmentComponent();

	// 장착/해제/복원이 일어날 때마다 방송. 장비창 UI와 HP 바가 구독한다
	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOnEquipmentChanged OnEquipmentChanged;

	// 인벤토리에 있는 아이템을 장착한다. 그 칸에 이미 낀 장비가 있으면 인벤토리로 돌려보낸다(맞바꿈). 성공 여부 반환
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool EquipFromInventory(FName ItemID);

	// 칸의 장비를 벗어 인벤토리로 돌려보낸다. 인벤토리가 가득 차 있으면 실패한다
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool Unequip(EEquipSlot Slot);

	UFUNCTION(BlueprintPure, Category = "Equipment")
	FName GetEquippedItem(EEquipSlot Slot) const;

	// 장착 중인 장비 전체가 주는 보너스
	UFUNCTION(BlueprintPure, Category = "Equipment")
	int32 GetTotalAttackBonus() const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	int32 GetTotalMaxHpBonus() const;

	// 아이템 ID에서 장비 정보를 만든다. 장비가 아니면 false
	bool GetEquipInfo(FName ItemID, FEquipInfo& OutInfo) const;

	// 아이템 아이콘. DataTable에 지정된 것이 있으면 그것을, 없으면 코드로 그린 검/방패 아이콘을 쓴다 (없으면 nullptr)
	class UTexture2D* ResolveIcon(FName ItemID) const;

	// 마우스를 올렸을 때 보여줄 설명 (이름 / 종류 / 설명 / 보너스 / 조작 안내). 장비가 아니면 빈 텍스트
	FText MakeTooltip(FName ItemID, bool bEquipped) const;

	// 새 게임의 시작 상태: 기본 검과 방패를 끼고 그 보너스를 적용한다
	void EquipDefaults();

	// 레벨 이동/리스폰 뒤 복원용. 모습만 되살리고 보너스는 다시 더하지 않는다 (이미 저장된 공격력/체력에 들어 있다)
	void RestoreEquipped(const TArray<FName>& Items);

	// 칸 순서(무기, 방패)대로의 장착 아이템 ID. 레벨 이동 때 저장하는 용도
	TArray<FName> GetEquippedItems() const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	static constexpr int32 SlotCount = 2;

	static int32 SlotIndex(EEquipSlot Slot) { return static_cast<int32>(Slot) - 1; }

	class UInventoryComponent* GetInventory() const;
	class ACharBase* GetOwnerCharacter() const;

	// 아이템의 보너스를 캐릭터 스탯에 더하거나(Sign=+1) 뺀다(Sign=-1)
	void ApplyBonus(const FEquipInfo& Info, int32 Sign);

	// 칸의 모습(기본 검/방패 본 표시 여부 + 붙는 모델)을 지금 장착 상태에 맞게 다시 맞춘다
	void RefreshVisual(EEquipSlot Slot);

	FName Equipped[SlotCount];

	// 칸별로 손에 붙여 둔 모델 컴포넌트 (기본 검/방패는 스킨에 구워져 있으므로 비어 있다)
	UPROPERTY()
	TObjectPtr<class UStaticMeshComponent> AttachedMeshes[SlotCount];
};
