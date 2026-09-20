#include "UI_Death.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/CoreStyle.h"

void UUI_Death::SetKillerName(const FText& KillerName)
{
	if (!DeathText)
		return;

	const FText Message = KillerName.IsEmpty() ? UnknownKillerMessage : FText::Format(KillerMessageFormat, KillerName);

	DeathText->SetText(Message);
}

void UUI_Death::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 첫 프레임에 화면이 한 번에 덮이지 않도록 투명하게 시작해서 NativeTick에서 서서히 올린다
	SetRenderOpacity(FadeInDuration > 0.f ? 0.f : 1.f);

	// 위젯 블루프린트로 이미 트리를 구성한 경우에는 트리는 건드리지 않고 버튼 연결만 한다
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildNativeTree();
	}

	if (ContinueButton)
	{
		// 죽는 순간 연타하던 공격 클릭이 버튼에 들어가 문구를 못 보고 넘어가지 않도록 ButtonEnableDelay까지 막아둔다
		ContinueButton->SetIsEnabled(false);
		ContinueButton->OnClicked.AddDynamic(this, &UUI_Death::HandleContinueClicked);
	}
}

void UUI_Death::BuildNativeTree()
{
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(BackdropColor);
	Backdrop->SetHorizontalAlignment(HAlign_Center);
	Backdrop->SetVerticalAlignment(VAlign_Center);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Backdrop->SetContent(Column);

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeathText"));

	FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", FontSize);
	Font.OutlineSettings.OutlineSize = 2;
	Font.OutlineSettings.OutlineColor = FLinearColor::Black;

	Text->SetFont(Font);
	Text->SetColorAndOpacity(FSlateColor(TextColor));
	Text->SetJustification(ETextJustify::Center);

	if (UVerticalBoxSlot* TextSlot = Column->AddChildToVerticalBox(Text))
	{
		TextSlot->SetHorizontalAlignment(HAlign_Center);
	}

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ContinueButton"));
	Button->SetBackgroundColor(FLinearColor(0.45f, 0.05f, 0.05f, 1.f));

	UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ContinueButtonText"));
	ButtonText->SetText(ContinueButtonText);
	ButtonText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", FontSize / 2));
	ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ButtonText->SetJustification(ETextJustify::Center);
	Button->AddChild(ButtonText);

	if (UVerticalBoxSlot* ButtonSlot = Column->AddChildToVerticalBox(Button))
	{
		ButtonSlot->SetHorizontalAlignment(HAlign_Center);
		ButtonSlot->SetPadding(FMargin(0.f, 40.f, 0.f, 0.f));
	}

	WidgetTree->RootWidget = Backdrop;
	DeathText = Text;
	ContinueButton = Button;
}

void UUI_Death::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bButtonEnabled && Elapsed >= FadeInDuration)
		return;

	Elapsed += InDeltaTime;

	if (Elapsed < FadeInDuration)
		SetRenderOpacity(FMath::Clamp(Elapsed / FadeInDuration, 0.f, 1.f));
	else
		SetRenderOpacity(1.f);

	if (!bButtonEnabled && Elapsed >= ButtonEnableDelay)
	{
		bButtonEnabled = true;

		if (ContinueButton)
			ContinueButton->SetIsEnabled(true);
	}
}

void UUI_Death::HandleContinueClicked()
{
	if (ContinueButton)
		ContinueButton->SetIsEnabled(false); // 더블클릭으로 리스폰이 두 번 걸리지 않게

	OnContinue.Broadcast();
}
