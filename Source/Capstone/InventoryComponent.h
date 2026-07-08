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
    // 같은 ID 슬롯이 있으면 수량 증가(MaxStack 준수), 없으면 슬롯 추가
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void AddItem(FName ItemID, int32 Count = 1);

    // 수량이 부족하면 false를 반환하고 아무 것도 바꾸지 않는다
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool RemoveItem(FName ItemID, int32 Count = 1);

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetItemCount(FName ItemID) const;

    // ItemDataTable에서 ItemID에 해당하는 행을 조회. 상점/강화 등 다른 시스템도 이 헬퍼로 조회한다
    const FItemData* GetItemData(FName ItemID) const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    TArray<FInventorySlot> Items;

    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    class UDataTable* ItemDataTable;

    // Add/Remove/골드 변화 때마다 방송. 인벤토리 UI·상점 UI 등이 구독한다
    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnInventoryChanged OnInventoryChanged;

public:
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void AddGold(int32 Amount);

    // 골드가 부족하면 false를 반환하고 아무 것도 바꾸지 않는다
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    bool TrySpendGold(int32 Amount);

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetGold() const { return Gold; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    int32 Gold = 0;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // 몬스터 처치 시 방송되는 전역 델리게이트를 구독해 골드를 지급
    void HandleMonsterKilled(FName MonsterID, int32 ExpReward, int32 GoldReward);

    FDelegateHandle MonsterKilledHandle;
};
