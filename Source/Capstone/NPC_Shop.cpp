#include "NPC_Shop.h"
#include "UI_Shop.h"
#include "Py_Grey.h"
#include "InventoryComponent.h"

ANPC_Shop::ANPC_Shop()
{
    NPCTypeTitle = TEXT("상점");
}

void ANPC_Shop::Interact_Implementation(AActor* Interactor)
{
    if (IsValid(ShopWidget))
        return;

    Super::Interact_Implementation(Interactor);

    if (!ShopWidgetClass)
        return;

    APy_Grey* Player = Cast<APy_Grey>(Interactor);
    if (!Player || !Player->InventoryComponent)
        return;

    ShopWidget = CreateWidget<UUI_Shop>(GetWorld(), ShopWidgetClass);
    if (ShopWidget)
    {
        CurrentInteractor = Interactor;

        ShopWidget->InitShop(ShopItemIDs, Player->InventoryComponent);
        ShopWidget->OnShopClosed.AddDynamic(this, &ANPC_Shop::CloseShop);
        ShopWidget->AddToViewport();

        Player->SetGameplayInputLocked(true);

        APlayerController* PC = Cast<APlayerController>(Player->GetController());
        if (PC)
        {
            PC->bShowMouseCursor = true;
            PC->SetInputMode(FInputModeGameAndUI());
        }
    }
}

void ANPC_Shop::CloseShop()
{
    if (ShopWidget)
    {
        ShopWidget->RemoveFromParent();
        ShopWidget = nullptr;
    }

    APy_Grey* Player = Cast<APy_Grey>(CurrentInteractor);
    if (Player)
    {
        Player->SetGameplayInputLocked(false);

        APlayerController* PC = Cast<APlayerController>(Player->GetController());
        if (PC)
        {
            PC->bShowMouseCursor = false;
            PC->SetInputMode(FInputModeGameOnly());
            // UI가 떠 있는 동안 마우스 클릭(NormalAttack 키) 등이 눌린 채로 걸려 있을 수 있어 초기화
            PC->FlushPressedKeys();
        }
    }

    CurrentInteractor = nullptr;
}
