#pragma once

#include "Engine/DataTable.h"
#include "ItemData.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
    Weapon,     
    Potion,     
    Material,   
    Loot        
};

USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    EItemType ItemType = EItemType::Material;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 BuyPrice = 0;

  
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 SellPrice = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 MaxStack = 99;

    int32 HealAmount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    TSoftObjectPtr<UTexture2D> Icon;
};
