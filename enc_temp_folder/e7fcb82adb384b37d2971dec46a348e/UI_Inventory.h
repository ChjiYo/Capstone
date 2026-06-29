#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_Inventory.generated.h"

UCLASS()
class CAPSTONE_API UUI_Inventory : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

public:
	UPROPERTY(meta = (BindWidget))
	class UButton* CloseButton;

	UPROPERTY(meta = (BindWidget))
	class UWrapBox* ItemWrapBox;

private:
	UFUNCTION()
	void OnCloseButtonClicked();
	
};
