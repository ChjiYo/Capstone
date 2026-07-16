#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_Enhance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnhanceClosed);

UCLASS()
class CAPSTONE_API UUI_Enhance : public UUserWidget
{
	GENERATED_BODY()

public:
    void InitEnhance(class APy_Grey* InPlayer);

    // 강화 창을 연 NPC가 바인딩해서 위젯 제거/입력모드 복구를 처리
    UPROPERTY(BlueprintAssignable, Category = "Enhance")
    FOnEnhanceClosed OnEnhanceClosed;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    void RefreshEnhanceUI();

    UFUNCTION()
    void HandleInventoryChanged();

    void HandleWeaponEnhanced(int32 NewLevel);

    UFUNCTION()
    void OnEnhanceButtonClicked();

    UFUNCTION()
    void OnCloseButtonClicked();

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* WeaponLevelText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* AttackPowerText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UImage* MaterialIcon;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* MaterialCostText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* GoldCostText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UButton* EnhanceButton;

    UPROPERTY(meta = (BindWidgetOptional))
    class UButton* CloseButton;

    UPROPERTY()
    class APy_Grey* Player;

    FDelegateHandle WeaponEnhancedHandle;
};
