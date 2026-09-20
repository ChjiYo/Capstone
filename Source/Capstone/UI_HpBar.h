#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_HpBar.generated.h"

UCLASS()
class CAPSTONE_API UUI_HpBar : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true", BindWidget) )
	class UProgressBar* HpBar;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* HpText;

	// 보호막 게이지(회색). HpBar 바로 뒤(오버레이에서 HpBar보다 먼저)에 깔리고, 채움이 "체력+보호막" 지점까지 차므로
	// HpBar 채움에 가려지지 않은 오른쪽 구간만 회색으로 보인다. 그래서 HpBar의 배경은 투명이어야 하고,
	// 보호막이 없을 때도 이 바가 트랙(배경)을 그려야 하므로 숨기지 않고 Percent만 0으로 둔다.
	// 몬스터 HP바처럼 이 위젯이 없는 BP에서는 무시됨
	UPROPERTY(meta = (BindWidgetOptional))
	class UProgressBar* ShieldBar;

	// 남은 보호막 수치 ("35 / 50"). 보호막이 없으면 Collapsed
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* ShieldText;

protected:
	virtual void NativeConstruct();
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime);

public:
	UFUNCTION(BlueprintCallable)
	void  SetHp(float fPercent);

	UFUNCTION(BlueprintCallable)
	void SetHpValue(int32 CurrentHp, int32 MaxHp);

	// 보호막 수치를 반영한다. MaxShield는 수치 텍스트("현재 / 최대")에만 쓰이고, 게이지 길이는 체력 대비 비율로 정해진다
	UFUNCTION(BlueprintCallable)
	void SetShieldValue(float CurrentShield, float MaxShield);

private:
	// HpBar/ShieldBar/ShieldText를 마지막으로 받은 체력·보호막 값으로 다시 그린다
	void RefreshBars();

	int32 CachedHp = 0;
	int32 CachedMaxHp = 0;
	float CachedShield = 0.f;
	float CachedShieldMax = 0.f;
};
