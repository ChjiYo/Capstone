#include "UI_Enhance.h"
#include "Py_Grey.h"
#include "InventoryComponent.h"
#include "ItemData.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"

void UUI_Enhance::InitEnhance(APy_Grey* InPlayer)
{
    Player = InPlayer;

    if (Player && Player->InventoryComponent)
    {
        Player->InventoryComponent->OnInventoryChanged.AddDynamic(this, &UUI_Enhance::HandleInventoryChanged);
    }

    WeaponEnhancedHandle = APy_Grey::OnWeaponEnhanced.AddUObject(this, &UUI_Enhance::HandleWeaponEnhanced);

    RefreshEnhanceUI();
}

void UUI_Enhance::NativeConstruct()
{
    Super::NativeConstruct();

    if (EnhanceButton)
    {
        EnhanceButton->OnClicked.AddDynamic(this, &UUI_Enhance::OnEnhanceButtonClicked);
    }

    if (CloseButton)
    {
        CloseButton->OnClicked.AddDynamic(this, &UUI_Enhance::OnCloseButtonClicked);
    }
}

void UUI_Enhance::NativeDestruct()
{
    if (Player && Player->InventoryComponent)
    {
        Player->InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UUI_Enhance::HandleInventoryChanged);
    }

    APy_Grey::OnWeaponEnhanced.Remove(WeaponEnhancedHandle);

    Super::NativeDestruct();
}

void UUI_Enhance::RefreshEnhanceUI()
{
    if (!Player || !Player->InventoryComponent)
        return;

    UInventoryComponent* Inventory = Player->InventoryComponent;

    if (WeaponLevelText)
    {
        WeaponLevelText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), Player->WeaponLevel)));
    }

    if (AttackPowerText)
    {
        AttackPowerText->SetText(FText::AsNumber(Player->GetAttackPower()));
    }

    int32 OwnedMaterial = Inventory->GetItemCount(Player->EnhanceMaterialID);
    int32 GoldCost = Player->GetNextEnhanceGoldCost();

    if (MaterialIcon)
    {
        if (const FItemData* MaterialData = Inventory->GetItemData(Player->EnhanceMaterialID))
        {
            if (!MaterialData->Icon.IsNull())
            {
                if (UTexture2D* IconTexture = MaterialData->Icon.LoadSynchronous())
                {
                    MaterialIcon->SetBrushFromTexture(IconTexture);
                }
            }
        }
    }

    if (MaterialCostText)
    {
        MaterialCostText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), OwnedMaterial, Player->EnhanceMaterialCount)));
    }

    if (GoldCostText)
    {
        GoldCostText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Inventory->GetGold(), GoldCost)));
    }

    if (EnhanceButton)
    {
        EnhanceButton->SetIsEnabled(Player->CanEnhanceWeapon());
    }
}

void UUI_Enhance::HandleInventoryChanged()
{
    RefreshEnhanceUI();
}

void UUI_Enhance::HandleWeaponEnhanced(int32 NewLevel)
{
    RefreshEnhanceUI();
}

void UUI_Enhance::OnEnhanceButtonClicked()
{
    if (Player)
    {
        Player->TryEnhanceWeapon();
    }
}

void UUI_Enhance::OnCloseButtonClicked()
{
    OnEnhanceClosed.Broadcast();
}
