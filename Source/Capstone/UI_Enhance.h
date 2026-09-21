#pragma once

#include "GameInfo.h"
#include "UI_PanelParts.h"
#include "Blueprint/UserWidget.h"
#include "UI_Enhance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnhanceClosed);

// 무기 강화 창. 지금 무기와 강화 단계(+N), 강화하면 오르는 공격력, 필요한 재료/골드(모자라면 붉게), 큰 [강화하기] 버튼을 한눈에 보여준다.
// 버튼 글자가 지금 못 하는 이유("재료가 부족합니다" 등)를 바로 알려주고, 성공하면 아이콘이 반짝이며 결과 메시지가 뜬다.
// bUseCodeBuiltLayout가 켜져 있으면(기본) 위젯 블루프린트(WBP_Enhance)의 트리를 버리고 골드 이중 테두리 판(인벤토리/장비창과 같은 모양)을
// 코드로 조립한다. 이때도 아래 BindWidgetOptional 이름은 채워지므로 NPC_Enhance 쪽 코드는 바뀐 게 없다.
// 끄면 예전처럼 WBP 레이아웃을 쓴다 (새 글자/아이콘 요소는 코드 조립 레이아웃에서만 채워진다)
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
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

    void RefreshEnhanceUI();

    UFUNCTION()
    void HandleInventoryChanged();

    void HandleWeaponEnhanced(int32 NewLevel);

    UFUNCTION()
    void OnEnhanceButtonClicked();

    UFUNCTION()
    void OnCloseButtonClicked();

    // 현재 강화 단계 ("+2")
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* WeaponLevelText;

    // 현재 공격력
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* AttackPowerText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UImage* MaterialIcon;

    // 재료 "보유 / 필요"
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* MaterialCostText;

    // 골드 "보유 / 필요"
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* GoldCostText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UButton* EnhanceButton;

    UPROPERTY(meta = (BindWidgetOptional))
    class UButton* CloseButton;

    // 켜면 WBP 트리 대신 코드로 조립한 레이아웃을 쓴다
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Layout")
    bool bUseCodeBuiltLayout = true;

    UPROPERTY()
    class APy_Grey* Player;

    FDelegateHandle WeaponEnhancedHandle;

private:
    void BuildNativeTree();

    // 단계 표시 마름모를 MaxLevel개로 다시 만든다
    void RebuildPips(int32 MaxLevel);

    void ShowToast(const FString& Message, bool bGood);
    void ShowHint();

    // 마름모/아이콘 칸은 강화 성공 때 반짝이는 연출(FlashRemaining)을 위해 매 틱 다시 칠한다
    void PaintHero(float Glow);

    PanelUI::FIconBox HeroIcon;
    PanelUI::FIconBox MaterialBox;
    PanelUI::FIconBox GoldBox;

    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> WeaponNameText;

    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> NextLevelText;

    UPROPERTY(Transient)
    TObjectPtr<class UHorizontalBox> PipRow;

    UPROPERTY(Transient)
    TArray<TObjectPtr<class UBorder>> PipShapes;

    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> AttackNextText;

    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> MaterialNameText;

    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> EnhanceLabel;

    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> ToastText;

    bool bBuiltNative = false;

    // 창이 뜬 직후 몇 틱 동안 키보드 포커스를 가져온다. 입력 모드를 바꾸는 순간 포커스가 게임 뷰포트로 가버려서 Esc/Enter가 안 먹기 때문
    int32 FocusTriesLeft = 0;

    float ToastRemaining = 0.f;
    float FlashRemaining = 0.f;
};
