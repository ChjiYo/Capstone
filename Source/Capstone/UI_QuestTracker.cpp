#include "UI_QuestTracker.h"
#include "UI_QuestStyle.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Blueprint/WidgetTree.h"

void UUI_QuestTracker::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 위젯 블루프린트의 트리를 통째로 코드 조립 트리로 교체한다. 디자이너 미리보기가 아니라 실제 인스턴스에서만
	if (bUseCodeBuiltLayout && WidgetTree && !IsDesignTime())
	{
		BuildNativeTree();
	}
}

void UUI_QuestTracker::BuildNativeTree()
{
	using namespace QuestUI;

	// 위젯 이름은 WBP_QuestTracker에 이미 있는 이름(TitleText 등)과 겹치면 안 된다. 같은 이름으로 만들면 WBP 쪽 옛 오브젝트 자리를 그대로 덮어쓰는데,
	// 버려진 옛 트리의 슬롯이 나중에 GC로 파괴될 때 그 슬롯이 아직 가리키는 새 텍스트 블록의 Slate 연결(MyTextBlock)까지 끊어 버린다.
	// 그러면 SetText가 화면에 반영되지 않아 진행 바만 갱신되고 문구가 "채집 3 / 5"에서 멈추므로, 코드로 만드는 위젯에는 "Native" 접두사를 붙인다
	// 루트: 화면 전체 Overlay 안에 패널을 좌상단 기준으로 놓는다. 패널 바깥은 클릭이 통과하도록 UpdateQuest가 SelfHitTestInvisible로 둔다
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));

	USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelWidth"));
	Width->SetWidthOverride(PanelWidth);

	UBorder* Panel = MakeBorder(WidgetTree, TEXT("Panel"), MakeBoxBrush(SRGB(0x0E, 0x0C, 0x0A, 0xD0), GoldDim, 1.f, 4.f), FMargin(1.f));
	Width->SetContent(Panel);

	// 왼쪽 골드 강조선 | 내용
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
	Panel->SetContent(Row);

	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(MakeLine(WidgetTree, TEXT("AccentBar"), Gold, 3.f, false)))
	{
		S->SetVerticalAlignment(VAlign_Fill);
		S->SetPadding(FMargin(0.f, 1.f));
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Column))
	{
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		S->SetPadding(FMargin(14.f, 10.f, 16.f, 12.f));
	}

	// 1) 라벨 줄: ◆ 퀘스트
	{
		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderRow"));

		if (UHorizontalBoxSlot* S = Header->AddChildToHorizontalBox(MakeDiamond(WidgetTree, TEXT("HeaderDiamond"), Gold, 6.f)))
		{
			S->SetVerticalAlignment(VAlign_Center);
			S->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		}

		UTextBlock* Label = MakeText(WidgetTree, TEXT("HeaderText"), 11, Gold, true, true, 250);
		Label->SetText(HeaderLabel);
		if (UHorizontalBoxSlot* S = Header->AddChildToHorizontalBox(Label))
		{
			S->SetVerticalAlignment(VAlign_Center);
		}

		Column->AddChildToVerticalBox(Header);
	}

	// 2) 제목
	{
		UTextBlock* Title = MakeText(WidgetTree, TEXT("NativeTitleText"), 19, GoldBright, true);
		Title->SetAutoWrapText(true);

		if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Title))
		{
			S->SetPadding(FMargin(0.f, 5.f, 0.f, 0.f));
		}

		TitleText = Title;
	}

	// 3) 구분선
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(MakeLine(WidgetTree, TEXT("Divider"), GoldDim, 1.f, true)))
	{
		S->SetPadding(FMargin(0.f, 8.f, 0.f, 8.f));
	}

	// 4) 설명
	{
		UTextBlock* Description = MakeText(WidgetTree, TEXT("NativeDescriptionText"), 14, Ivory, false);
		Description->SetAutoWrapText(true);
		Column->AddChildToVerticalBox(Description);

		DescriptionText = Description;
	}

	// 5) 진행 영역: 진행 문구 + 진행 바
	{
		UVerticalBox* Area = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ProgressArea"));

		UTextBlock* Progress = MakeText(WidgetTree, TEXT("NativeProgressText"), 14, Gold, true);
		Area->AddChildToVerticalBox(Progress);

		UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ProgressBar"));

		FProgressBarStyle BarStyle;
		BarStyle.BackgroundImage = MakeBoxBrush(FLinearColor(0.f, 0.f, 0.f, 0.55f), SRGB(0xCB, 0xA2, 0x4A, 0x55), 1.f, 3.f);
		BarStyle.FillImage = MakeBoxBrush(FLinearColor::White, FLinearColor::Transparent, 0.f, 3.f);
		BarStyle.MarqueeImage = BarStyle.FillImage;
		Bar->SetWidgetStyle(BarStyle);
		Bar->SetFillColorAndOpacity(Gold);
		Bar->SetPercent(0.f);

		USizeBox* BarHeight = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ProgressBarHeight"));
		BarHeight->SetHeightOverride(7.f);
		BarHeight->SetContent(Bar);

		if (UVerticalBoxSlot* S = Area->AddChildToVerticalBox(BarHeight))
		{
			S->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
		}

		if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Area))
		{
			S->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
		}

		ProgressText = Progress;
		ProgressBar = Bar;
		ProgressArea = Area;
	}

	if (UOverlaySlot* PanelSlot = Root->AddChildToOverlay(Width))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Left);
		PanelSlot->SetVerticalAlignment(VAlign_Top);
		PanelSlot->SetPadding(FMargin(ScreenMargin.X, ScreenMargin.Y, 0.f, 0.f));
	}

	WidgetTree->RootWidget = Root;
}

void UUI_QuestTracker::UpdateQuest(const FQuestData& InQuestData)
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (TitleText)
	{
		TitleText->SetText(InQuestData.QuestTitle);
	}

	if (DescriptionText)
	{
		DescriptionText->SetText(InQuestData.QuestDescription);
	}

	const bool bHasProgress = InQuestData.bIsCompleted || InQuestData.HasObjective();

	if (ProgressText)
	{
		FText ProgressStr;

		if (InQuestData.bIsCompleted)
		{
			ProgressStr = NSLOCTEXT("UI_QuestTracker", "Completed", "완료!");
		}
		else if (InQuestData.HasObjective())
		{
			FText Verb;
			switch (InQuestData.ObjectiveType)
			{
				case EQuestObjectiveType::Kill:    Verb = NSLOCTEXT("UI_QuestTracker", "Kill", "처치"); break;
				case EQuestObjectiveType::Gather:  Verb = NSLOCTEXT("UI_QuestTracker", "Gather", "채집"); break;
				case EQuestObjectiveType::Enhance: Verb = NSLOCTEXT("UI_QuestTracker", "Enhance", "강화"); break;
				default: break;
			}

			ProgressStr = FText::Format(
				NSLOCTEXT("UI_QuestTracker", "ProgressFormat", "{0} {1} / {2}"),
				Verb, InQuestData.CurrentCount, InQuestData.TargetCount);
		}

		ProgressText->SetText(ProgressStr);
		ProgressText->SetColorAndOpacity(FSlateColor(InQuestData.bIsCompleted ? QuestUI::Success : QuestUI::Gold));
	}

	if (ProgressBar)
	{
		const float Percent = InQuestData.bIsCompleted ? 1.f
			: (InQuestData.HasObjective() ? FMath::Clamp(static_cast<float>(InQuestData.CurrentCount) / InQuestData.TargetCount, 0.f, 1.f) : 0.f);

		ProgressBar->SetPercent(Percent);
		ProgressBar->SetFillColorAndOpacity(InQuestData.bIsCompleted ? QuestUI::Success : QuestUI::Gold);
	}

	// 목표가 없는 대화형 퀘스트는 진행 문구/바를 통째로 접어 패널이 텅 빈 줄 없이 짧아지게 한다
	if (ProgressArea)
	{
		ProgressArea->SetVisibility(bHasProgress ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UUI_QuestTracker::ClearQuest()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
