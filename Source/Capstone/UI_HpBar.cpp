#include "UI_HpBar.h"
#include "Components/ProgressBar.h"

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
