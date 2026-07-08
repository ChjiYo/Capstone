#include "ItemBase.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "InventoryComponent.h"
#include "Py_Grey.h"
#include "DrawDebugHelpers.h"

AItemBase::AItemBase()
{
    PrimaryActorTick.bCanEverTick = false;

    ItemVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ItemVFX"));
    RootComponent = ItemVFX;
    ItemVFX->SetAutoActivate(false);
    
    ItemVFX->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ItemVFX->SetGenerateOverlapEvents(false);

    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
    CollisionComponent->SetupAttachment(RootComponent);
    CollisionComponent->InitSphereRadius(150.f);
    CollisionComponent->SetCollisionProfileName(TEXT("Trigger"));

    CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AItemBase::OnOverlapBegin);
    CollisionComponent->OnComponentEndOverlap.AddDynamic(this, &AItemBase::OnOverlapEnd);

    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> LootVFXFinder(TEXT("/Script/Niagara.NiagaraSystem'/Game/Imortal_Loot_Drop_VFX/Niagara/NS_Loot_Drop_3.NS_Loot_Drop_3'"));
    if (LootVFXFinder.Succeeded())
        LootDropVFX = LootVFXFinder.Object;

    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HealVFXFinder(TEXT("/Script/Niagara.NiagaraSystem'/Game/Imortal_Loot_Drop_VFX/Niagara/NS_Loot_Drop.NS_Loot_Drop'"));
    if (HealVFXFinder.Succeeded())
        HealDropVFX = HealVFXFinder.Object;

    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> WeaponVFXFinder(TEXT("/Script/Niagara.NiagaraSystem'/Game/Imortal_Loot_Drop_VFX/Niagara/NS_Loot_Drop_2.NS_Loot_Drop_2'"));
    if (WeaponVFXFinder.Succeeded())
        WeaponDropVFX = WeaponVFXFinder.Object;
}

void AItemBase::BeginPlay()
{
    Super::BeginPlay();

    UNiagaraSystem* SelectedVFX = LootDropVFX;

    switch (ItemType)
    {
    case EItemType::Weapon:
        SelectedVFX = WeaponDropVFX;
        break;

    case EItemType::Potion:
    case EItemType::Material:
        SelectedVFX = HealDropVFX;
        break;

    case EItemType::Loot:
    default:
        SelectedVFX = LootDropVFX;
        break;
    }

    if (SelectedVFX && ItemVFX)
    {
        ItemVFX->SetAsset(SelectedVFX);
        ItemVFX->Activate(true);
    }
}

void AItemBase::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    APy_Grey* Player = Cast<APy_Grey>(OtherActor);
    if (Player)
    {
        Player->SetInteractableItem(this);
    }
    
}

void AItemBase::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
  
    APy_Grey* Player = Cast<APy_Grey>(OtherActor);
    if (Player)
    {
        Player->SetInteractableItem(nullptr);
    }
    
}

void AItemBase::PickupItem(ACharacter* Picker)
{
    if (Picker)
    {
        if (Picker)
        {
            APy_Grey* PlayerCharacter = Cast<APy_Grey>(Picker);

            if (PlayerCharacter && PlayerCharacter->InventoryComponent)
            {
                PlayerCharacter->InventoryComponent->AddItem(ItemID);

                Destroy();
            }
        }

    }
}