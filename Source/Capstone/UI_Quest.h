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

    // 퀘스트를 발급한 NPC가 바인딩해서 수락/거절 결과(보상 지급 등)를 처리
    UPROPERTY(BlueprintAssignable, Category = "Quest")
    FOnQuestAccepted OnQuestAccepted;

    UPROPERTY(BlueprintAssignable, Category = "Quest")
    FOnQuestDeclined OnQuestDeclined;

protected:
    virtual void NativeConstruct() override;

    // BindWidgetOptional: 이 이름의 위젯이 아직 UMG에 없어도 블루프린트 컴파일은 깨지지 않는다.
    // 위젯 블루프린트(BP_UI_NPC_Quest)에서 아래와 정확히 같은 이름으로 위젯을 만들면 자동 연결됨
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
