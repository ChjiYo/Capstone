#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_SkillBar.generated.h"

UCLASS()
class CAPSTONE_API UUI_SkillBar : public UUserWidget
{
	GENERATED_BODY()

public:
	// APy_Grey::BeginPlay에서 위젯 생성 직후 호출: 소유 플레이어 저장 + 슬롯 아이콘/키 표기 1회 세팅
	void InitSkillBar(class APy_Grey* InOwnerPlayer);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* SlotAIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	class UProgressBar* SlotACooldownBar;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* SlotACooldownText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* SlotAKeyText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* SlotBIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	class UProgressBar* SlotBCooldownBar;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* SlotBCooldownText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* SlotBKeyText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* SlotCIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	class UProgressBar* SlotCCooldownBar;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* SlotCCooldownText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* SlotCKeyText;

private:
	TWeakObjectPtr<class APy_Grey> OwnerPlayer;

	void UpdateSlot(int32 SlotIndex, class UProgressBar* CooldownBar, class UTextBlock* CooldownText);
};
