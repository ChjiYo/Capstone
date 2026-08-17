#include "UI_Dialogue.h"
#include "DialogueData.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"

void UUI_Dialogue::NativeConstruct()
{
	Super::NativeConstruct();

	if (NextButton)
	{
		NextButton->OnClicked.AddDynamic(this, &UUI_Dialogue::OnNextClicked);
	}
}

void UUI_Dialogue::SetLine(const FDialogueLine& Line)
{
	FullLine = Line.LineText.ToString();
	RevealedChars = 0.f;
	bRevealing = CharsPerSecond > 0.f && FullLine.Len() > 0;

	const bool bNarration = Line.bIsNarration || Line.SpeakerName.IsEmpty();

	if (SpeakerText)
	{
		SpeakerText->SetVisibility(bNarration ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		SpeakerText->SetText(Line.SpeakerName);
	}

	if (NarrationBackground)
	{
		NarrationBackground->SetVisibility(bNarration ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (PortraitImage)
	{
		UTexture2D* Portrait = Line.PortraitIcon.IsNull() ? nullptr : Line.PortraitIcon.LoadSynchronous();
		if (!bNarration && Portrait)
		{
			PortraitImage->SetBrushFromTexture(Portrait);
			PortraitImage->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			PortraitImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (LineText)
	{
		LineText->SetText(bRevealing ? FText::GetEmpty() : FText::FromString(FullLine));
	}

	if (ContinueHintText)
	{
		ContinueHintText->SetVisibility(bRevealing ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
	}
}

void UUI_Dialogue::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bRevealing || !LineText)
		return;

	RevealedChars += CharsPerSecond * InDeltaTime;

	const int32 NumChars = FMath::Clamp(FMath::FloorToInt(RevealedChars), 0, FullLine.Len());
	LineText->SetText(FText::FromString(FullLine.Left(NumChars)));

	if (NumChars >= FullLine.Len())
	{
		bRevealing = false;

		if (ContinueHintText)
		{
			ContinueHintText->SetVisibility(ESlateVisibility::Visible);
		}
	}
}

void UUI_Dialogue::OnNextClicked()
{
	if (bRevealing)
	{
		// 아직 글자가 다 나오지 않았으면 전체를 즉시 표시만 하고, 다음 줄로는 넘어가지 않는다
		bRevealing = false;
		RevealedChars = static_cast<float>(FullLine.Len());

		if (LineText)
		{
			LineText->SetText(FText::FromString(FullLine));
		}

		if (ContinueHintText)
		{
			ContinueHintText->SetVisibility(ESlateVisibility::Visible);
		}
		return;
	}

	OnAdvanceRequested.ExecuteIfBound();
}
