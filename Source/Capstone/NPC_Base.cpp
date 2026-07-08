#include "NPC_Base.h"
#include "Py_Grey.h"

ANPC_Base::ANPC_Base()
{
    PrimaryActorTick.bCanEverTick = false;
    NPCName = TEXT("Default NPC");
    NPCTypeTitle = TEXT("Hans");

    InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
    InteractionSphere->SetupAttachment(GetRootComponent());
    InteractionSphere->InitSphereRadius(200.f);
}

void ANPC_Base::BeginPlay()
{
    Super::BeginPlay();

    InteractionSphere->SetCollisionProfileName(TEXT("Trigger"));
    InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ANPC_Base::OnOverlapBegin);
    InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ANPC_Base::OnOverlapEnd);
}

void ANPC_Base::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ANPC_Base::Interact_Implementation(AActor* Interactor)
{
    LOG(TEXT("%s: Hello."), *NPCName);
}

void ANPC_Base::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    APy_Grey* Player = Cast<APy_Grey>(OtherActor);

    if (Player)
    {
        Player->SetInteractableNPC(this);
    }
}


void ANPC_Base::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    APy_Grey* Player = Cast<APy_Grey>(OtherActor);

    if (Player)
    {
        Player->SetInteractableNPC(nullptr);
    }
}