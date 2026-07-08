#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_QuestTracker.generated.h"

UCLASS()
class CAPSTONE_API UUI_QuestTracker : public UUserWidget
{
	GENERATED_BODY()

public:
	// 활성 퀘스트 정보로 트래커를 갱신하고 보이게 한다
	void UpdateQuest(const FQuestData& InQuestData);

	// 활성 퀘스트가 없을 때 트래커를 숨긴다
	void ClearQuest();

protected:
	// 위젯 블루프린트에 아래 이름과 정확히 같은 이름의 위젯을 만들어야 자동으로 연결됨.
	// BindWidgetOptional이라 위젯이 없어도 블루프린트 컴파일은 깨지지 않고, 해당 항목만 비어 표시됨
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* DescriptionText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* ProgressText;
};
