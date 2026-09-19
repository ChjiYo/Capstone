#include "UI_HpBar.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UUI_HpBar::NativeConstruct()
{
	Super::NativeConstruct();

	HpBar = Cast<UProgressBar>(GetWidgetFromName(TEXT("HpBar")));
	
}

void UUI_HpBar::NativeTick(const FGeometry & MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

void UUI_HpBar::SetHp(float fPercent)
{
	if (IsValid(HpBar))
	{
		HpBar->SetPercent(fPercent);
	}
}

void UUI_HpBar::SetHpValue(int32 CurrentHp, int32 MaxHp)
{
	CurrentHp = FMath::Max(CurrentHp, 0);

	SetHp(MaxHp > 0 ? static_cast<float>(CurrentHp) / static_cast<float>(MaxHp) : 0.f);

	if (IsValid(HpText))
	{
		HpText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), CurrentHp, MaxHp)));
	}
}
