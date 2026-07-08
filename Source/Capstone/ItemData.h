#pragma once

#include "Engine/DataTable.h"
#include "ItemData.generated.h"

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

    // 행 이름(RowName)이 곧 ItemID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    EItemType ItemType = EItemType::Material;

    // 상점 구매가 (0이면 비매품)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 BuyPrice = 0;

    // 상점 판매가 (0이면 판매 불가)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 SellPrice = 0;

    // 스택 상한 (장비는 1)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 MaxStack = 99;

    // Potion 전용: 사용 시 회복량
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 HealAmount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    TSoftObjectPtr<UTexture2D> Icon;
};
