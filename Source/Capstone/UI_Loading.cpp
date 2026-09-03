#include "UI_Loading.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "UObject/UObjectGlobals.h"

void UUI_Loading::BeginLoading()
{
	ElapsedTime = 0.f;
	bLoadFinished = false;
	LastPercent = 0.f;
}

void UUI_Loading::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	ElapsedTime += InDeltaTime;

	float TargetPercent;
	if (bLoadFinished)
	{
		TargetPercent = 100.f;
	}
	else
	{
		float RealPercent = -1.f;
		if (!TargetLevelPackage.IsNone())
		{
			RealPercent = GetAsyncLoadPercentage(TargetLevelPackage);
		}

		if (RealPercent >= 0.f)
		{
			TargetPercent = RealPercent;
		}
		else
		{
			// 시간이 지날수록 느려지며 95%에 근접하는 가짜 진행률 (100%는 로딩이 끝나야만 표시)
			constexpr float EstimatedDuration = 3.0f;
			TargetPercent = 95.f * (1.f - FMath::Exp(-ElapsedTime / EstimatedDuration));
		}
	}

	LastPercent = FMath::FInterpTo(LastPercent, TargetPercent, InDeltaTime, bLoadFinished ? 8.f : 3.f);

	if (LoadingBar)
	{
		LoadingBar->SetPercent(LastPercent / 100.f);
	}

	if (PercentText)
	{
		PercentText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(LastPercent))));
	}
}
