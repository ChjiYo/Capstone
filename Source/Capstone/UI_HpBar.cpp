#include "UI_HpBar.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UUI_HpBar::NativeConstruct()
{
	Super::NativeConstruct();

	HpBar = Cast<UProgressBar>(GetWidgetFromName(TEXT("HpBar")));

	RefreshBars();
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

	CachedHp = CurrentHp;
	CachedMaxHp = MaxHp;

	if (MaxHp <= 0)
	{
		SetHp(0.f);
	}

	RefreshBars();

	if (IsValid(HpText))
	{
		HpText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), CurrentHp, MaxHp)));
	}
}

void UUI_HpBar::SetShieldValue(float CurrentShield, float MaxShield)
{
	CachedShield = FMath::Max(CurrentShield, 0.f);
	CachedShieldMax = MaxShield;

	RefreshBars();
}

void UUI_HpBar::RefreshBars()
{
	const float Hp = static_cast<float>(CachedHp);
	const bool bHasShield = CachedShield > 0.f;

	// 체력+보호막이 최대 체력을 넘으면 오른쪽이 잘려 나가지 않도록 그 합을 바 전체 길이로 삼는다.
	// (풀피에서 보호막을 켜도 게이지가 보이고, 체력바는 그만큼 살짝 짧아 보인다 - 실제 수치는 HpText 참고)
	const float Total = FMath::Max(static_cast<float>(CachedMaxHp), Hp + CachedShield);

	if (IsValid(HpBar) && Total > 0.f)
	{
		HpBar->SetPercent(Hp / Total);
	}

	if (IsValid(ShieldBar))
	{
		// ShieldBar가 HpBar 뒤에 깔려 있으므로 (체력+보호막) 지점까지 채우면 체력 채움 오른쪽으로 보호막만큼 회색이 삐져나온다
		ShieldBar->SetPercent((bHasShield && Total > 0.f) ? FMath::Clamp((Hp + CachedShield) / Total, 0.f, 1.f) : 0.f);
	}

	if (IsValid(ShieldText))
	{
		ShieldText->SetVisibility(bHasShield ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

		if (bHasShield)
		{
			ShieldText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(CachedShield), FMath::CeilToInt(CachedShieldMax))));
		}
	}
}
