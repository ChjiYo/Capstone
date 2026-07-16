#include "UI_SkillBar.h"
#include "Py_Grey.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UUI_SkillBar::InitSkillBar(APy_Grey* InOwnerPlayer)
{
	OwnerPlayer = InOwnerPlayer;

	if (SlotAKeyText) SlotAKeyText->SetText(FText::FromString(TEXT("1")));
	if (SlotBKeyText) SlotBKeyText->SetText(FText::FromString(TEXT("2")));
	if (SlotCKeyText) SlotCKeyText->SetText(FText::FromString(TEXT("3")));

	if (!OwnerPlayer.IsValid())
		return;

	const FSkillSlotData* Slots = OwnerPlayer->GetSkillSlots();
	UImage* Icons[3] = { SlotAIcon, SlotBIcon, SlotCIcon };

	for (int32 i = 0; i < 3; ++i)
	{
		if (!Icons[i] || Slots[i].Icon.IsNull())
			continue;

		UTexture2D* Texture = Slots[i].Icon.LoadSynchronous();
		if (Texture)
			Icons[i]->SetBrushFromTexture(Texture);
	}
}

void UUI_SkillBar::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!OwnerPlayer.IsValid())
		return;

	UpdateSlot(0, SlotACooldownBar, SlotACooldownText);
	UpdateSlot(1, SlotBCooldownBar, SlotBCooldownText);
	UpdateSlot(2, SlotCCooldownBar, SlotCCooldownText);
}

void UUI_SkillBar::UpdateSlot(int32 SlotIndex, UProgressBar* CooldownBar, UTextBlock* CooldownText)
{
	float Remaining = OwnerPlayer->GetSkillCooldownRemaining(SlotIndex);
	float Cooldown = OwnerPlayer->GetSkillCooldown(SlotIndex);

	if (CooldownBar)
	{
		float Percent = Cooldown > 0.f ? Remaining / Cooldown : 0.f;
		CooldownBar->SetPercent(Percent);
	}

	if (CooldownText)
	{
		if (Remaining > 0.f)
			CooldownText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), Remaining)));
		else
			CooldownText->SetText(FText::GetEmpty());
	}
}
