#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_Quest.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnQuestAccepted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnQuestDeclined);

UCLASS()
class CAPSTONE_API UUI_Quest : public UUserWidget
{
	GENERATED_BODY()

public:
    void SetQuestData(const FQuestData& InQuestData);

    UPROPERTY(BlueprintAssignable, Category = "Quest")
    FOnQuestAccepted OnQuestAccepted;

    UPROPERTY(BlueprintAssignable, Category = "Quest")
    FOnQuestDeclined OnQuestDeclined;

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* TitleText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* DescriptionText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* RewardText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UButton* AcceptButton;

    UPROPERTY(meta = (BindWidgetOptional))
    class UButton* DeclineButton;

    UFUNCTION()
    void OnAcceptClicked();

    UFUNCTION()
    void OnDeclineClicked();

    FQuestData CurrentQuestData;
};
