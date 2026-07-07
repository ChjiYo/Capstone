#include "UI_Inventory.h"
#include "InventoryComponent.h"
#include "UI_InventorySlot.h"
#include "Components/Button.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
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
	// 우클릭, 좌클릭 등을 판별할 수 있습니다.
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		UE_LOG(LogTemp, Warning, TEXT("인벤토리 배경을 우클릭했습니다."));
		// 여기서 FReply::Handled()를 반환하면 클릭 이벤트가 여기서 처리되고 끝납니다.
		return FReply::Handled();
	}
	bOpenUI = false;
	// 특별한 처리가 없다면 부모의 기본 동작을 수행합니다.
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UUI_Inventory::OnCloseButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("인벤토리 닫기 버튼 클릭됨!"));

	// 1. 화면에서 위젯 제거
	RemoveFromParent();

	// 2. 플레이어 컨트롤러를 가져와서 마우스 커서를 숨기고 게임 입력 모드로 복귀
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC)
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
}