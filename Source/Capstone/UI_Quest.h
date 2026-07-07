#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_Quest.generated.h"

UCLASS()
class CAPSTONE_API UUI_Quest : public UUserWidget
{
	GENERATED_BODY()
	
public:
    void SetQuestData(const FQuestData& InQuestData);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* TitleText;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* DescriptionText;

    UPROPERTY(meta = (BindWidget))
    class UButton* AcceptButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* DeclineButton;

    UFUNCTION()
    void OnAcceptClicked();

    UFUNCTION()
    void OnDeclineClicked();

    FQuestData CurrentQuestData;
};
