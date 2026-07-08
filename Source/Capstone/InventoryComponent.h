#pragma once

#include "GameInfo.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CAPSTONE_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

public:
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void AddItem(FName NewItemID);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    TArray<FName> Items;

public:
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void AddGold(int32 Amount);

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
