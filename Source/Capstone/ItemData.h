#pragma once

#include "Engine/DataTable.h"
#include "ItemData.generated.h"

// 장비창에서 어느 칸에 끼우는 아이템인지. None이면 장비가 아니다
UENUM(BlueprintType)
enum class EEquipSlot : uint8
{
    None,
    Weapon,     // 오른손 검
    Shield      // 왼손 방패
};

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 HealAmount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    TSoftObjectPtr<UTexture2D> Icon;

    // ===== 장비 정보 (EquipSlot이 None이 아닐 때만 의미가 있다) =====
    // 이 값들을 DataTable 행에 채우면 그 값이 쓰이고, 비워둔 행(EquipSlot=None)은 EquipmentData.cpp의 코드 정의를 ItemID로 찾아 쓴다

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    EEquipSlot EquipSlot = EEquipSlot::None;

    // 장착 중일 때 공격력에 더해지는 값
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    int32 AttackBonus = 0;

    // 장착 중일 때 최대 체력에 더해지는 값
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    int32 MaxHpBonus = 0;

    // 장착 시 캐릭터 손에 붙는 모델. 비워두면 눈에 보이는 모델 없이 수치만 적용된다
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    TSoftObjectPtr<UStaticMesh> EquipMesh;

    // EquipMesh를 붙일 캐릭터 메시의 소켓/본 이름
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FName EquipSocket = NAME_None;

    // 소켓 기준 상대 위치/회전/크기
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FTransform EquipOffset = FTransform::Identity;
};
