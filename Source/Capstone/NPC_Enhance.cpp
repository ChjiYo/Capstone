#include "NPC_Enhance.h"
#include "UI_Enhance.h"
#include "Py_Grey.h"

ANPC_Enhance::ANPC_Enhance()
{}

void ANPC_Enhance::Interact_Implementation(AActor* Interactor)
{
    if (IsValid(EnhanceWidget))
        return;

    Super::Interact_Implementation(Interactor);

    if (!EnhanceWidgetClass)
        return;

    APy_Grey* Player = Cast<APy_Grey>(Interactor);
    if (!Player || !Player->InventoryComponent)
        return;

    EnhanceWidget = CreateWidget<UUI_Enhance>(GetWorld(), EnhanceWidgetClass);
    if (EnhanceWidget)
    {
        CurrentInteractor = Interactor;

        EnhanceWidget->InitEnhance(Player);
        EnhanceWidget->OnEnhanceClosed.AddDynamic(this, &ANPC_Enhance::CloseEnhance);
        EnhanceWidget->AddToViewport();

        Player->SetGameplayInputLocked(true);

        APlayerController* PC = Cast<APlayerController>(Player->GetController());
        if (PC)
        {
            PC->bShowMouseCursor = true;
            PC->SetInputMode(FInputModeGameAndUI());
        }
    }
}

void ANPC_Enhance::CloseEnhance()
{
    if (EnhanceWidget)
    {
        EnhanceWidget->RemoveFromParent();
        EnhanceWidget = nullptr;
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
