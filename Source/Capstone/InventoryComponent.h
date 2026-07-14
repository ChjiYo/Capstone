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

    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetItemCount(FName ItemID) const;

    const FItemData* GetItemData(FName ItemID) const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    TArray<FInventorySlot> Items;

    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    class UDataTable* ItemDataTable;

  
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
