#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_BossHpBar.generated.h"

// 화면 상단에 표시되는 보스 전용 HP 바 (8단계). BindWidgetOptional이라 짝 위젯 BP에 요소가 없어도 컴파일은 깨지지 않는다
UCLASS()
class CAPSTONE_API UUI_BossHpBar : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetBossHp(float fPercent);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetPhase(int32 Phase);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	class UProgressBar* HpBar;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* PhaseText;
};
