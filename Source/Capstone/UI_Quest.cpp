
#include "UI_Quest.h"
#include "UI_QuestStyle.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Blueprint/WidgetTree.h"

void UUI_Quest::SetQuestData(const FQuestData& InQuestData)
{
	CurrentQuestData = InQuestData;

	if (TitleText)
	{
		TitleText->SetText(CurrentQuestData.QuestTitle);
	}

	if (DescriptionText)
	{
		DescriptionText->SetText(CurrentQuestData.QuestDescription);
	}

	if (RewardText)
	{
		RewardText->SetText(BuildRewardText(CurrentQuestData));
	}
}

FText UUI_Quest::BuildRewardText(const FQuestData& Data) const
{
	// 코드 조립 레이아웃은 "보상" 라벨이 따로 있으므로 접두어를 붙이지 않는다. WBP 레이아웃일 때만 예전처럼 "Reward: "를 붙여 라벨 역할을 대신한다
	TArray<FText> Parts;

	if (Data.Reward.RewardExp > 0)
		Parts.Add(FText::Format(NSLOCTEXT("UI_Quest", "RewardExp", "경험치 +{0}"), Data.Reward.RewardExp));

	if (Data.Reward.RewardGold > 0)
		Parts.Add(FText::Format(NSLOCTEXT("UI_Quest", "RewardGold", "골드 +{0}"), Data.Reward.RewardGold));

	if (!Data.Reward.RewardItemID.IsNone() && Data.Reward.RewardItemCount > 0)
	{
		Parts.Add(FText::Format(NSLOCTEXT("UI_Quest", "RewardItem", "{0} x{1}"),
			FText::FromName(Data.Reward.RewardItemID), Data.Reward.RewardItemCount));
	}

	if (Parts.Num() == 0)
		return NSLOCTEXT("UI_Quest", "RewardNone", "없음");

	FText Joined = Parts[0];
	for (int32 i = 1; i < Parts.Num(); ++i)
	{
		Joined = FText::Format(NSLOCTEXT("UI_Quest", "RewardJoin", "{0}  ·  {1}"), Joined, Parts[i]);
	}

	if (bUseCodeBuiltLayout)
		return Joined;

	return FText::Format(NSLOCTEXT("UI_Quest", "RewardFormat", "Reward: {0}"), Joined);
}

void UUI_Quest::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 위젯 블루프린트의 트리를 통째로 코드 조립 트리로 교체한다. 디자이너 미리보기가 아니라 실제 인스턴스에서만
	if (bUseCodeBuiltLayout && WidgetTree && !IsDesignTime())
	{
		BuildNativeTree();
	}

	// 처음엔 투명하게 두고 NativeTick에서 서서히 올린다
	if (IntroDuration > 0.f)
	{
		SetRenderOpacity(0.f);
	}
}

void UUI_Quest::BuildNativeTree()
{
	using namespace QuestUI;

	// 루트: 화면 전체를 덮는 Overlay. 첫 번째 자식이 딤 배경, 두 번째가 중앙 패널
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));

	UBorder* Dim = MakeBorder(WidgetTree, TEXT("Dim"), MakeBoxBrush(FLinearColor(0.f, 0.f, 0.f, 0.55f), FLinearColor::Transparent, 0.f, 0.f));
	if (UOverlaySlot* DimSlot = Root->AddChildToOverlay(Dim))
	{
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// 바깥 프레임(골드 테두리) -> 안쪽 패널(따뜻한 갈색 톤, 얇은 보조 테두리) 이중 구조
	USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelWidth"));
	Width->SetWidthOverride(PanelWidth);

	UBorder* Frame = MakeBorder(WidgetTree, TEXT("Frame"), MakeBoxBrush(PanelFill, Gold, 2.f, 6.f), FMargin(6.f));
	Width->SetContent(Frame);

	UBorder* Panel = MakeBorder(WidgetTree, TEXT("Panel"), MakeBoxBrush(PanelInner, GoldDim, 1.f, 4.f), FMargin(34.f, 22.f, 34.f, 28.f));
	Frame->SetContent(Panel);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Panel->SetContent(Column);

	// 1) 헤더: ──── ◆ 퀘스트 ◆ ────
	{
		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderRow"));

		auto AddLine = [&](const TCHAR* Name)
		{
			if (UHorizontalBoxSlot* S = Header->AddChildToHorizontalBox(MakeLine(WidgetTree, Name, GoldDim, 1.f, true)))
			{
				S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				S->SetVerticalAlignment(VAlign_Center);
			}
		};

		auto AddDiamond = [&](const TCHAR* Name)
		{
			if (UHorizontalBoxSlot* S = Header->AddChildToHorizontalBox(MakeDiamond(WidgetTree, Name, Gold, 7.f)))
			{
				S->SetVerticalAlignment(VAlign_Center);
				S->SetPadding(FMargin(10.f, 0.f));
			}
		};

		UTextBlock* HeaderText = MakeText(WidgetTree, TEXT("HeaderText"), 13, Gold, true, true, 300);
		HeaderText->SetText(HeaderLabel);

		AddLine(TEXT("HeaderLineL"));
		AddDiamond(TEXT("HeaderDiamondL"));

		if (UHorizontalBoxSlot* S = Header->AddChildToHorizontalBox(HeaderText))
		{
			S->SetVerticalAlignment(VAlign_Center);
		}

		AddDiamond(TEXT("HeaderDiamondR"));
		AddLine(TEXT("HeaderLineR"));

		Column->AddChildToVerticalBox(Header);
	}

	// 2) 제목
	{
		UTextBlock* Title = MakeText(WidgetTree, TEXT("TitleText"), 30, GoldBright, true);
		Title->SetJustification(ETextJustify::Center);
		Title->SetAutoWrapText(true);

		if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Title))
		{
			S->SetHorizontalAlignment(HAlign_Center);
			S->SetPadding(FMargin(0.f, 18.f, 0.f, 14.f));
		}

		TitleText = Title;
	}

	// 3) 구분선
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(MakeLine(WidgetTree, TEXT("TitleDivider"), GoldDim, 1.f, true)))
	{
		S->SetPadding(FMargin(40.f, 0.f));
	}

	// 4) 본문. 한두 줄짜리 설명이 답답해 보이지 않게 최소 높이를 준다
	{
		UTextBlock* Description = MakeText(WidgetTree, TEXT("DescriptionText"), 17, Ivory, false);
		Description->SetAutoWrapText(true);
		Description->SetJustification(ETextJustify::Center);

		USizeBox* DescBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DescriptionBox"));
		DescBox->SetMinDesiredHeight(84.f);
		DescBox->SetContent(Description);

		if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(DescBox))
		{
			S->SetPadding(FMargin(6.f, 18.f, 6.f, 18.f));
			S->SetVerticalAlignment(VAlign_Center);
		}

		DescriptionText = Description;
	}

	// 5) 보상 박스: [보상]  경험치 +100 · 골드 +50 · 아이템 x1
	{
		UBorder* RewardBox = MakeBorder(WidgetTree, TEXT("RewardBox"), MakeBoxBrush(InsetFill, GoldDim, 1.f, 3.f), FMargin(18.f, 11.f));

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RewardRow"));
		RewardBox->SetContent(Row);

		UTextBlock* Label = MakeText(WidgetTree, TEXT("RewardLabelText"), 14, Gold, true, true, 150);
		Label->SetText(RewardLabel);
		if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Label))
		{
			S->SetVerticalAlignment(VAlign_Center);
			S->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
		}

		UTextBlock* Reward = MakeText(WidgetTree, TEXT("RewardText"), 16, Ivory, false);
		Reward->SetAutoWrapText(true);
		if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Reward))
		{
			S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			S->SetVerticalAlignment(VAlign_Center);
		}

		Column->AddChildToVerticalBox(RewardBox);
		RewardText = Reward;
	}

	// 6) 버튼: 가운데에 서로 붙여서 둔다 (수락이 강조색, 거절은 톤을 죽임)
	{
		UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));

		auto MakeButton = [&](const TCHAR* Name, const FText& Label, const FButtonStyle& Style, const FLinearColor& LabelColor) -> UButton*
		{
			USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName(*FString::Printf(TEXT("%sSize"), Name)));
			Size->SetWidthOverride(150.f);
			Size->SetHeightOverride(44.f);

			UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(Name));
			Button->SetStyle(Style);
			Size->SetContent(Button);

			UTextBlock* Text = MakeText(WidgetTree, *FString::Printf(TEXT("%sText"), Name), 18, LabelColor, true, true, 200);
			Text->SetText(Label);
			Text->SetJustification(ETextJustify::Center);

			if (UPanelSlot* PanelSlot = Button->AddChild(Text))
			{
				if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(PanelSlot))
				{
					ButtonSlot->SetHorizontalAlignment(HAlign_Center);
					ButtonSlot->SetVerticalAlignment(VAlign_Center);
				}
			}

			if (UHorizontalBoxSlot* S = Buttons->AddChildToHorizontalBox(Size))
			{
				S->SetPadding(FMargin(7.f, 0.f));
			}

			return Button;
		};

		AcceptButton = MakeButton(TEXT("AcceptButton"), AcceptLabel,
			MakeButtonStyle(SRGB(0x7A, 0x5A, 0x22), SRGB(0x99, 0x74, 0x2C), SRGB(0x5E, 0x44, 0x18), Gold, GoldBright),
			SRGB(0xFF, 0xF3, 0xD0));

		DeclineButton = MakeButton(TEXT("DeclineButton"), DeclineLabel,
			MakeButtonStyle(SRGB(0x1F, 0x1B, 0x17), SRGB(0x30, 0x2A, 0x24), SRGB(0x14, 0x11, 0x0E), SRGB(0x6B, 0x63, 0x55), Muted),
			Muted);

		if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Buttons))
		{
			S->SetHorizontalAlignment(HAlign_Center);
			S->SetPadding(FMargin(0.f, 24.f, 0.f, 0.f));
		}
	}

	if (UOverlaySlot* PanelSlot = Root->AddChildToOverlay(Width))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
	}

	PanelRoot = Width;
	WidgetTree->RootWidget = Root;
}

void UUI_Quest::NativeConstruct()
{
	Super::NativeConstruct();

	if (AcceptButton)
	{
		AcceptButton->OnClicked.AddUniqueDynamic(this, &UUI_Quest::OnAcceptClicked);
	}

	if (DeclineButton)
	{
		DeclineButton->OnClicked.AddUniqueDynamic(this, &UUI_Quest::OnDeclineClicked);
	}
}

void UUI_Quest::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (IntroDuration <= 0.f || IntroElapsed >= IntroDuration)
		return;

	IntroElapsed = FMath::Min(IntroElapsed + InDeltaTime, IntroDuration);

	// ease-out: 처음에 빠르게 나타나고 끝에서 느려진다
	const float Alpha = FMath::Clamp(IntroElapsed / IntroDuration, 0.f, 1.f);
	const float Eased = 1.f - FMath::Square(1.f - Alpha);

	SetRenderOpacity(Eased);

	if (PanelRoot)
		PanelRoot->SetRenderTranslation(FVector2D(0.f, (1.f - Eased) * 14.f));
}

void UUI_Quest::OnAcceptClicked()
{
	OnQuestAccepted.Broadcast();
	RemoveFromParent();
}

void UUI_Quest::OnDeclineClicked()
{
	OnQuestDeclined.Broadcast();
	RemoveFromParent();
}
