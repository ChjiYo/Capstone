#include "UI_Dialogue.h"
#include "DialogueData.h"
#include "CapstoneSFX.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"

void UUI_Dialogue::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (NextButton)
	{
		NextButton->OnClicked.AddUniqueDynamic(this, &UUI_Dialogue::OnNextClicked);
	}
}

void UUI_Dialogue::SetLine(const FDialogueLine& Line)
{
	FullLine = Line.LineText.ToString();
	RevealedChars = 0.f;
	LastBlipChars = 0;

	// 화자 이름으로 3가지 블립 음높이 중 하나를 고정해서, 사람마다 목소리 톤이 다르게 들리게 한다. 나레이션은 낮고 조용하게
	bBlipNarration = Line.bIsNarration || Line.SpeakerName.IsEmpty();
	BlipVariant = bBlipNarration ? 0 : static_cast<int32>(GetTypeHash(Line.SpeakerName.ToString()) % 3);
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

	// 글자 2개가 나올 때마다 타자기 블립 (공백/문장부호만 나온 구간은 건너뜀)
	if (NumChars >= LastBlipChars + 2)
	{
		LastBlipChars = NumChars;

		if (FullLine.IsValidIndex(NumChars - 1) && !FChar::IsWhitespace(FullLine[NumChars - 1]) && !FChar::IsPunct(FullLine[NumChars - 1]))
			CapstoneSFX::Play2D(this, ESFX::UI_DialogueBlip, bBlipNarration ? 0.5f : 1.f, bBlipNarration ? 0.8f : 1.f, BlipVariant);
	}

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
