#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_Dialogue.generated.h"

struct FDialogueLine;

// 하단 대사창 (11단계). StoryComponent가 생성·소유하고 SetLine으로 매 줄을 갱신한다
UCLASS()
class CAPSTONE_API UUI_Dialogue : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetLine(const FDialogueLine& Line);

	// StoryComponent가 바인딩한다 (버튼 클릭 → 다음 줄 요청)
	FSimpleDelegate OnAdvanceRequested;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 위젯 블루프린트(WBP_Dialogue)에 아래와 정확히 같은 이름으로 위젯을 만들어야 자동으로 연결됨
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* SpeakerText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* LineText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* PortraitImage;

	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* NarrationBackground;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* ContinueHintText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* NextButton;

	// 0이면 타자기 효과 없이 즉시 전체 표시
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	float CharsPerSecond = 40.f;

	UFUNCTION()
	void OnNextClicked();

private:
	FString FullLine;
	float RevealedChars = 0.f;
	bool bRevealing = false;
};
