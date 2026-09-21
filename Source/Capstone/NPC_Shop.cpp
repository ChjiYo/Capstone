#include "NPC_Shop.h"
#include "UI_Shop.h"
#include "Py_Grey.h"
#include "InventoryComponent.h"
#include "CapstoneSFX.h"

ANPC_Shop::ANPC_Shop()
{
    NPCTypeTitle = TEXT("상점");
}

void ANPC_Shop::Interact_Implementation(AActor* Interactor)
{
    // 창이 열려 있을 때 다시 E를 누르면 닫는다 (E는 게임 입력으로 그대로 전달된다)
    if (IsValid(ShopWidget))
    {
        CloseShop();
        return;
    }

    APy_Grey* Player = Cast<APy_Grey>(Interactor);
    if (!Player || !Player->InventoryComponent)
        return;

    APlayerController* PC = Cast<APlayerController>(Player->GetController());
    if (!PC)
        return;

    Super::Interact_Implementation(Interactor);

    // 블루프린트에 상점 위젯 클래스를 안 넣어 뒀어도 코드로 조립한 창이 뜬다
    const TSubclassOf<UUI_Shop> WidgetClass = ShopWidgetClass ? ShopWidgetClass : TSubclassOf<UUI_Shop>(UUI_Shop::StaticClass());

    ShopWidget = CreateWidget<UUI_Shop>(PC, WidgetClass);
    if (ShopWidget)
    {
        CurrentInteractor = Interactor;

        ShopWidget->InitShop(ShopItemIDs, Player->InventoryComponent);
        ShopWidget->OnShopClosed.AddDynamic(this, &ANPC_Shop::CloseShop);
        ShopWidget->AddToViewport();
        CapstoneSFX::Play2D(this, ESFX::UI_Open);

        // 입력 잠금 + 마우스 커서. 창이 열린 채 인벤토리/장비창을 여닫아도 풀리지 않도록 플레이어가 관리한다
        Player->SetNPCPanelOpen(true);

        // Esc로 닫으려면 키보드 포커스가 이 창에 있어야 한다
        ShopWidget->SetKeyboardFocus();
    }
}

void ANPC_Shop::CloseShop()
{
    if (ShopWidget)
    {
        ShopWidget->RemoveFromParent();
        ShopWidget = nullptr;
        CapstoneSFX::Play2D(this, ESFX::UI_Close);
    }

    // 인벤토리/장비창이 아직 열려 있으면 입력 잠금과 커서를 유지하고, 다 닫혔으면 게임 입력으로 되돌린다
    APy_Grey* Player = Cast<APy_Grey>(CurrentInteractor);
    if (Player)
    {
        Player->SetNPCPanelOpen(false);
    }

    CurrentInteractor = nullptr;
}
