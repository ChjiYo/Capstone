#include "GatherableObject.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "InventoryComponent.h"
#include "Py_Grey.h"
#include "TimerManager.h"

FOnItemGathered AGatherableObject::OnItemGathered;

AGatherableObject::AGatherableObject()
{
}

void AGatherableObject::PickupItem(ACharacter* Picker)
{
    if (!Picker)
        return;

    APy_Grey* PlayerCharacter = Cast<APy_Grey>(Picker);
    if (!PlayerCharacter || !PlayerCharacter->InventoryComponent)
        return;

    PlayerCharacter->InventoryComponent->AddItem(ItemID, GatherCount);
    OnItemGathered.Broadcast(ItemID, GatherCount);

    LOG(TEXT("Gathered %s x%d"), *ItemID.ToString(), GatherCount);

    // 파괴 대신 비활성화: 메시/VFX 숨김 + 콜리전 해제, RespawnTime 뒤에 원복
    SetActorHiddenInGame(true);

    if (CollisionComponent)
        CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (ItemVFX)
        ItemVFX->Deactivate();

    if (RespawnTime > 0.f)
    {
        GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AGatherableObject::Respawn, RespawnTime, false);
    }
    else
    {
        Destroy();
    }
}

void AGatherableObject::Respawn()
{
    SetActorHiddenInGame(false);

    if (CollisionComponent)
        CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

    if (ItemVFX)
        ItemVFX->Activate(true);
}
