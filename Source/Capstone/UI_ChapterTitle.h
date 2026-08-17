#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_ChapterTitle.generated.h"

// 챕터 타이틀 카드 (11단계). 퀘스트 수락 직후 잠깐 표시됐다가 자동으로 사라진다
UCLASS()
class CAPSTONE_API UUI_ChapterTitle : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Story")
	void SetChapter(const FText& InLabel, const FText& InTitle);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* ChapterLabelText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* ChapterTitleText;

	// 위젯 블루프린트에 정확히 이 이름으로 애니메이션을 만들면 자동 연결됨. 없으면 페이드 없이 즉시 표시된다
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	class UWidgetAnimation* FadeAnim;
};
