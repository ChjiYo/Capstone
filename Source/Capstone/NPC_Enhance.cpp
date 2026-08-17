#include "NPC_Enhance.h"
#include "UI_Enhance.h"
#include "Py_Grey.h"
#include "StoryComponent.h"

ANPC_Enhance::ANPC_Enhance()
{}

void ANPC_Enhance::Interact_Implementation(AActor* Interactor)
{
    if (IsValid(EnhanceWidget))
        return;

    APy_Grey* Player = Cast<APy_Grey>(Interactor);
    if (!Player || !Player->InventoryComponent)
        return;

    UStoryComponent* Story = Player->StoryComponent;
    if (Story && Story->IsPlaying())
        return; // 다른 대사 재생 중 재진입 차단 (강화창이 대사 위에 겹쳐 뜨는 것을 방지)

    CurrentInteractor = Interactor;

    if (Story && FirstMeetDialogueLines.Num() > 0)
    {
        // 처음 만났을 때만 대사 → 끝나면 강화창. 두 번째부터는 곧장 강화창
        if (Story->PlayDialogueOnce(FirstMeetBeatID, FirstMeetDialogueLines, true,
                FSimpleDelegate::CreateUObject(this, &ANPC_Enhance::OpenEnhanceWidget)))
            return;
    }

    OpenEnhanceWidget();
}

void ANPC_Enhance::OpenEnhanceWidget()
{
    if (!CurrentInteractor)
        return;

    Super::Interact_Implementation(CurrentInteractor);

    if (!EnhanceWidgetClass)
        return;

    APy_Grey* Player = Cast<APy_Grey>(CurrentInteractor);
    if (!Player || !Player->InventoryComponent)
        return;

    EnhanceWidget = CreateWidget<UUI_Enhance>(GetWorld(), EnhanceWidgetClass);
    if (EnhanceWidget)
    {
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
