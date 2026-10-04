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
	// 버튼 연결은 여기서 한 번만 한다. 이 위젯은 StoryComponent가 재사용하면서 RemoveFromParent/AddToViewport를
	// 반복하고, 그때마다 NativeConstruct가 다시 불려 AddDynamic이 중복되면 엔진 ensure가 터진다
	virtual void NativeOnInitialized() override;
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

	// 타자기 블립 상태: 마지막으로 소리를 낸 글자 수 / 화자별 블립 종류 / 나레이션 여부
	int32 LastBlipChars = 0;
	int32 BlipVariant = 0;
	bool bBlipNarration = false;
};
