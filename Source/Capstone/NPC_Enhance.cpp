#include "NPC_Enhance.h"
#include "UI_Enhance.h"
#include "Py_Grey.h"
#include "StoryComponent.h"
#include "CapstoneSFX.h"

ANPC_Enhance::ANPC_Enhance()
{
    NPCTypeTitle = TEXT("강화");
}

void ANPC_Enhance::Interact_Implementation(AActor* Interactor)
{
    // 창이 열려 있을 때 다시 E를 누르면 닫는다 (E는 게임 입력으로 그대로 전달된다)
    if (IsValid(EnhanceWidget))
    {
        CloseEnhance();
        return;
    }

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

    APy_Grey* Player = Cast<APy_Grey>(CurrentInteractor);
    if (!Player || !Player->InventoryComponent)
        return;

    APlayerController* PC = Cast<APlayerController>(Player->GetController());
    if (!PC)
        return;

    // 블루프린트에 강화 위젯 클래스를 안 넣어 뒀어도 코드로 조립한 창이 뜬다
    const TSubclassOf<UUI_Enhance> WidgetClass = EnhanceWidgetClass ? EnhanceWidgetClass : TSubclassOf<UUI_Enhance>(UUI_Enhance::StaticClass());

    EnhanceWidget = CreateWidget<UUI_Enhance>(PC, WidgetClass);
    if (EnhanceWidget)
    {
        EnhanceWidget->InitEnhance(Player);
        EnhanceWidget->OnEnhanceClosed.AddDynamic(this, &ANPC_Enhance::CloseEnhance);
        EnhanceWidget->AddToViewport();
        CapstoneSFX::Play2D(this, ESFX::UI_Open);

        // 입력 잠금 + 마우스 커서. 창이 열린 채 인벤토리/장비창을 여닫아도 풀리지 않도록 플레이어가 관리한다
        Player->SetNPCPanelOpen(true);

        // Esc/Enter를 받으려면 키보드 포커스가 이 창에 있어야 한다
        EnhanceWidget->SetKeyboardFocus();
    }
}

void ANPC_Enhance::CloseEnhance()
{
    if (EnhanceWidget)
    {
        EnhanceWidget->RemoveFromParent();
        EnhanceWidget = nullptr;
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
