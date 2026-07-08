#include "UI_Inventory.h"
#include "InventoryComponent.h"
#include "UI_InventorySlot.h"
#include "Components/Button.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

void UUI_Inventory::RefreshInventoryUI(UInventoryComponent* InventoryComp)
{
	if (!InventoryComp || !InventoryWrapBox) return;

	for (UUI_InventorySlot* pSlot : SlotArray)
	{
		if (pSlot)
		{
			pSlot->ClearSlot();
		}
	}

	for (int32 i = 0; i < InventoryComp->Items.Num(); ++i)
	{
		if (i < SlotArray.Num())
		{
			if (SlotArray[i])
			{
				const FInventorySlot& InvenSlot = InventoryComp->Items[i];
				SlotArray[i]->InitSlot(InvenSlot, InventoryComp->GetItemData(InvenSlot.ItemID));
			}
		}
	}

	if (GoldText)
	{
		GoldText->SetText(FText::FromString(FString::Printf(TEXT("%d Gold"), InventoryComp->GetGold())));
	}
}

void UUI_Inventory::NativeConstruct()
{
	Super::NativeConstruct();

	bOpenUI = true;
	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UUI_Inventory::OnCloseButtonClicked);
	}

	if (InventoryWrapBox && InventorySlotClass)
	{
		InventoryWrapBox->ClearChildren();
		SlotArray.Empty();

		for (int32 i = 0; i < MaxInventorySlots; ++i)
		{
			UUI_InventorySlot* NewSlot = CreateWidget<UUI_InventorySlot>(GetWorld(), InventorySlotClass);
			if (NewSlot)
			{
				NewSlot->ClearSlot(); 
				InventoryWrapBox->AddChildToWrapBox(NewSlot);
				SlotArray.Add(NewSlot); 
			}
		}
	}
}

FReply UUI_Inventory::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{

	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		return FReply::Handled();
	}
	bOpenUI = false;

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UUI_Inventory::OnCloseButtonClicked()
{
	RemoveFromParent();

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC)
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
}