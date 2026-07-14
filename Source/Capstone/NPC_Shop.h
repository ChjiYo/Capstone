#pragma once

#include "NPC_Base.h"
#include "NPC_Shop.generated.h"

UCLASS()
class CAPSTONE_API ANPC_Shop : public ANPC_Base
{
	GENERATED_BODY()

public:
    ANPC_Shop();

protected:
    // 이 상점이 파는 아이템 ID 목록 (DT_Items의 RowName)
    UPROPERTY(EditAnywhere, Category = "Shop")
    TArray<FName> ShopItemIDs;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUI_Shop> ShopWidgetClass;

    UPROPERTY()
    class UUI_Shop* ShopWidget;

    // 상점 창을 띄운 시점의 플레이어. 닫기 콜백에서 입력모드 복구에 사용
    UPROPERTY()
    AActor* CurrentInteractor;

    virtual void Interact_Implementation(AActor* Interactor) override;

    UFUNCTION()
    void CloseShop();
};
