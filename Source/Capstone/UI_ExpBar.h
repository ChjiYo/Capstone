#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_ExpBar.generated.h"

UCLASS()
class CAPSTONE_API UUI_ExpBar : public UUserWidget
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, Category = "Experience")
	void SetExp(int32 CurrentExp, int32 RequiredExp, int32 Level);

protected:
	
	
	class UProgressBar* ExpBar;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* LevelText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* ExpText;
};
