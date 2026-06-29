#include "ItemBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "InventoryComponent.h"
#include "Py_Grey.h" 

AItemBase::AItemBase()
{
    PrimaryActorTick.bCanEverTick = false;

    ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
    RootComponent = ItemMesh;

    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
    CollisionComponent->SetupAttachment(RootComponent);
    CollisionComponent->InitSphereRadius(150.f); 
    CollisionComponent->SetCollisionProfileName(TEXT("Trigger"));

    CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AItemBase::OnOverlapBegin);
    CollisionComponent->OnComponentEndOverlap.AddDynamic(this, &AItemBase::OnOverlapEnd);
}

void AItemBase::BeginPlay()
{
    Super::BeginPlay();
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