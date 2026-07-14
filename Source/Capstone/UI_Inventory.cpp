#include "UI_Inventory.h"
#include "InventoryComponent.h"
#include "UI_InventorySlot.h"
#include "Py_Grey.h"
#include "Components/Button.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

void UUI_Inventory::RefreshInventoryUI(UInventoryComponent* InInventoryComp)
{
	if (!InInventoryComp || !InventoryWrapBox) return;

	if (InventoryComp != InInventoryComp)
	{
		if (InventoryComp)
		{
			InventoryComp->OnInventoryChanged.RemoveDynamic(this, &UUI_Inventory::HandleInventoryChanged);
		}

		InventoryComp = InInventoryComp;
		InventoryComp->OnInventoryChanged.AddDynamic(this, &UUI_Inventory::HandleInventoryChanged);
	}

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

void UUI_Inventory::HandleInventoryChanged()
{
	RefreshInventoryUI(InventoryComp);
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

void UUI_Inventory::NativeDestruct()
{
	if (InventoryComp)
	{
		InventoryComp->OnInventoryChanged.RemoveDynamic(this, &UUI_Inventory::HandleInventoryChanged);
	}

	Super::NativeDestruct();
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
		// UI가 떠 있는 동안 마우스 클릭(NormalAttack 키) 등이 눌린 채로 걸려 있을 수 있어 초기화
		PC->FlushPressedKeys();

		if (APy_Grey* Player = Cast<APy_Grey>(PC->GetPawn()))
		{
			Player->SetGameplayInputLocked(false);
		}
	}
}