#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_Inventory.generated.h"

UCLASS()
class CAPSTONE_API UUI_Inventory : public UUserWidget
{
    GENERATED_BODY()

public:
    // 인벤토리 데이터를 받아 UI를 갱신하는 함수
    void RefreshInventoryUI(class UInventoryComponent* InventoryComp);

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    UFUNCTION()
    void OnCloseButtonClicked();

    // 닫기 버튼
    UPROPERTY(meta = (BindWidget))
    class UButton* CloseButton;

    // 아이템들이 들어갈 랩박스 (블루프린트의 변수 이름과 동일해야 함)
    UPROPERTY(meta = (BindWidget))
    class UWrapBox* InventoryWrapBox;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUserWidget> InventorySlotClass;
	
};
