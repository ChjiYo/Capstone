#include "NPC_Base.h"

ANPC_Base::ANPC_Base()
{
    PrimaryActorTick.bCanEverTick = false; 

    NPCName = TEXT("Default NPC");
    NPCTypeTitle = TEXT("Hans");
}

void ANPC_Base::BeginPlay()
{
    Super::BeginPlay();
}

void ANPC_Base::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ANPC_Base::Interact(AActor* Interactor)
{
    LOG(TEXT("%s: Hello."), *NPCName);
}