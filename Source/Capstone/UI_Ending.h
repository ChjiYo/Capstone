#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_Ending.generated.h"

// 보스 처치 후 표시되는 엔딩 화면 (8단계). BindWidgetOptional이라 짝 위젯 BP(WBP_Ending)에 요소가 없어도 컴파일은 깨지지 않는다
UCLASS()
class CAPSTONE_API UUI_Ending : public UUserWidget
{
	GENERATED_BODY()

public:
	// 플레이 결과 요약 표시 + 재시작용 시작 레벨 이름 보관. APy_Grey가 위젯 생성 직후 호출
	UFUNCTION(BlueprintCallable, Category = "UI")
	void InitEnding(FName InStartLevelName, int32 FinalLevel, int32 FinalGold);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* SummaryText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* RestartButton;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* QuitButton;

	UFUNCTION()
	void OnRestartClicked();

	UFUNCTION()
	void OnQuitClicked();

	FName StartLevelName;
};
