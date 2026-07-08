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
				SlotArray[i]->InitSlot(InventoryComp->Items[i]);
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
	// ��Ŭ��, ��Ŭ�� ���� �Ǻ��� �� �ֽ��ϴ�.
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		UE_LOG(LogTemp, Warning, TEXT("�κ��丮 ����� ��Ŭ���߽��ϴ�."));
		// ���⼭ FReply::Handled()�� ��ȯ�ϸ� Ŭ�� �̺�Ʈ�� ���⼭ ó���ǰ� �����ϴ�.
		return FReply::Handled();
	}
	bOpenUI = false;
	// Ư���� ó���� ���ٸ� �θ��� �⺻ ������ �����մϴ�.
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UUI_Inventory::OnCloseButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("�κ��丮 �ݱ� ��ư Ŭ����!"));

	// 1. ȭ�鿡�� ���� ����
	RemoveFromParent();

	// 2. �÷��̾� ��Ʈ�ѷ��� �����ͼ� ���콺 Ŀ���� ����� ���� �Է� ���� ����
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC)
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
}