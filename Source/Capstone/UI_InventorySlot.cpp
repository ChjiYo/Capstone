#include "UI_InventorySlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UUI_InventorySlot::InitSlot(FName InItemID)
{
	CurrentItemID = InItemID;

	if (ItemNameText)
	{
		ItemNameText->SetText(FText::FromName(InItemID));
	}

	// ItemID를 검색해 아이템의 아이콘 이미지(UTexture2D*)를 가져온 뒤 아래와 같이 세팅합니다.
	// if (ItemIcon && ItemTexture) { ItemIcon->SetBrushFromTexture(ItemTexture); }
}