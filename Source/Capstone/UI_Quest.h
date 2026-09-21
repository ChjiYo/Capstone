#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_Quest.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnQuestAccepted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnQuestDeclined);

// NPC 퀘스트 수락/거절 다이얼로그.
// bUseCodeBuiltLayout가 켜져 있으면(기본) 위젯 블루프린트(WBP_UI_NPC_Quest)에 배치된 트리를 버리고
// NativeOnInitialized에서 RPG풍 패널(골드 이중 테두리 + 보상 박스 + 붙어 있는 수락/거절 버튼)을 코드로 조립한다.
// 이때도 아래 바인딩 이름(TitleText 등)은 그대로 채워지므로 NPC_Quest 쪽 코드는 바뀐 게 없다.
// 위젯 디자이너에서 직접 만든 레이아웃을 쓰고 싶으면 WBP의 클래스 기본값에서 bUseCodeBuiltLayout를 끈다
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
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

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

protected:
    // 켜면 WBP 트리 대신 코드로 조립한 레이아웃을 쓴다
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Layout")
    bool bUseCodeBuiltLayout = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Layout")
    FText HeaderLabel = INVTEXT("퀘스트");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Layout")
    FText RewardLabel = INVTEXT("보상");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Layout")
    FText AcceptLabel = INVTEXT("수락");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Layout")
    FText DeclineLabel = INVTEXT("거절");

    // 패널 가로 폭 (1080p 기준 유닛)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Layout")
    float PanelWidth = 620.f;

    // 창이 뜰 때 페이드인 + 살짝 떠오르는 연출 시간(초). 0이면 연출 없음
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Layout")
    float IntroDuration = 0.22f;

private:
    void BuildNativeTree();

    // 보상 문구를 "경험치 +100 · 골드 +50 · 아이템ID x2" 형태로 만든다 (0인 항목은 뺀다)
    FText BuildRewardText(const FQuestData& Data) const;

    // 떠오르는 연출을 적용할 중앙 패널 (딤 배경은 움직이지 않는다). 코드 조립 레이아웃에서만 채워진다
    UPROPERTY(Transient)
    TObjectPtr<class UWidget> PanelRoot;

    float IntroElapsed = 0.f;
};
