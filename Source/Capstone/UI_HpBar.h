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

protected:
	virtual void NativeConstruct();
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime);

public:
	UFUNCTION(BlueprintCallable)
	void  SetHp(float fPercent);
};
