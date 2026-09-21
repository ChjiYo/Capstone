#pragma once

#include "GameInfo.h"
#include "ItemData.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

USTRUCT(BlueprintType)
struct FInventorySlot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    FName ItemID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    int32 Count = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CAPSTONE_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

public:
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void AddItem(FName ItemID, int32 Count = 1);

    bool RemoveItem(FName ItemID, int32 Count = 1);

    // 같은 아이템 여러 스택을 모두 합친 수량
    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetItemCount(FName ItemID) const;

    // DataTable 행을 돌려준다. 표에 없는 아이템이라도 코드에 정의된 장비(EquipmentData.cpp)면 그 정보로 채운 행을 돌려준다
    const FItemData* GetItemData(FName ItemID) const;

    // DataTable에 실제로 있는 행만 (코드 정의 장비로 채운 행은 제외). 장비 정보 병합에서 어느 쪽이 원본인지 구분하는 데 쓴다
    const FItemData* FindTableRow(FName ItemID) const;

    // 인벤토리 창이 한 번에 보여주는 칸 수. 장착 해제로 돌아오는 아이템이 창에서 안 보이는 일이 없도록 장비 쪽에서 이 한도를 지킨다
    static constexpr int32 MaxSlots = 20;

    // Count개를 더 넣을 자리가 있는가 (기존 스택의 남은 자리 + 빈 칸)
    bool CanAddItem(FName ItemID, int32 Count = 1) const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    TArray<FInventorySlot> Items;

    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    class UDataTable* ItemDataTable;

    // 표에 없는 코드 정의 장비를 FItemData로 바꿔 둔 캐시 (GetItemData가 포인터를 돌려주므로 보관이 필요하다)
    mutable TMap<FName, FItemData> SyntheticItems;

  
    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnInventoryChanged OnInventoryChanged;

public:
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void AddGold(int32 Amount);

   
    bool TrySpendGold(int32 Amount);

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetGold() const { return Gold; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    int32 Gold = 10000;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void HandleMonsterKilled(FName MonsterID, int32 ExpReward, int32 GoldReward);

    FDelegateHandle MonsterKilledHandle;
};
