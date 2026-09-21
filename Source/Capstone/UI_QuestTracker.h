#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_QuestTracker.generated.h"

// 화면 한쪽에 항상 떠 있는 활성 퀘스트 트래커 (제목 / 설명 / 진행도 + 진행 바).
// bUseCodeBuiltLayout가 켜져 있으면(기본) 위젯 블루프린트(WBP_QuestTracker)의 트리를 버리고
// NativeOnInitialized에서 반투명 패널 + 골드 강조선 + 진행 바를 코드로 조립한다.
// 바인딩 이름(TitleText/DescriptionText/ProgressText)은 그대로 채워지므로 Py_Grey 쪽 코드는 바뀐 게 없다.
// 위젯 디자이너에서 직접 만든 레이아웃을 쓰고 싶으면 WBP의 클래스 기본값에서 bUseCodeBuiltLayout를 끈다
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
	virtual void NativeOnInitialized() override;

	// 위젯 블루프린트에 아래 이름과 정확히 같은 이름의 위젯을 만들어야 자동으로 연결됨.
	// BindWidgetOptional이라 위젯이 없어도 블루프린트 컴파일은 깨지지 않고, 해당 항목만 비어 표시됨
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* DescriptionText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* ProgressText;

	// 목표가 있는 퀘스트의 진행 바. 코드 조립 레이아웃에만 있다 (WBP에 같은 이름으로 두면 그쪽도 채워진다)
	UPROPERTY(meta = (BindWidgetOptional))
	class UProgressBar* ProgressBar;

	// 진행 문구 + 진행 바를 묶은 영역. 목표가 없는 대화형 퀘스트에서는 통째로 접는다
	UPROPERTY(meta = (BindWidgetOptional))
	class UWidget* ProgressArea;

protected:
	// 켜면 WBP 트리 대신 코드로 조립한 레이아웃을 쓴다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "QuestTracker|Layout")
	bool bUseCodeBuiltLayout = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "QuestTracker|Layout")
	FText HeaderLabel = INVTEXT("퀘스트");

	// 패널 가로 폭 (1080p 기준 유닛)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "QuestTracker|Layout")
	float PanelWidth = 320.f;

	// 화면 가장자리와의 간격. X는 좌측 기준, Y는 상단 기준
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "QuestTracker|Layout")
	FVector2D ScreenMargin = FVector2D(28.0, 210.0);

private:
	void BuildNativeTree();
};
