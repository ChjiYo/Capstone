#include "UI_ItemDrag.h"
#include "UI_QuestStyle.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

UWidget* ItemDrag::MakeVisual(UObject* Outer, const FText& Name, UTexture2D* Icon)
{
	UBorder* Card = NewObject<UBorder>(Outer);
	Card->SetBrush(QuestUI::MakeBoxBrush(QuestUI::PanelInner, QuestUI::Gold, 2.f, 6.f));
	Card->SetPadding(FMargin(6.f));
	Card->SetRenderOpacity(0.9f);

	if (Icon)
	{
		USizeBox* Box = NewObject<USizeBox>(Outer);
		Box->SetWidthOverride(56.f);
		Box->SetHeightOverride(56.f);

		UImage* Image = NewObject<UImage>(Outer);
		Image->SetBrushFromTexture(Icon, false);
		Box->SetContent(Image);

		Card->SetContent(Box);
	}
	else
	{
		UTextBlock* Text = NewObject<UTextBlock>(Outer);
		Text->SetText(Name);
		Text->SetColorAndOpacity(FSlateColor(QuestUI::Ivory));
		Text->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));

		Card->SetContent(Text);
	}

	return Card;
}
