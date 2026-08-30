#include "NPC_Base.h"
#include "Py_Grey.h"
#include "Components/WidgetComponent.h"
#include "UI_NPC_Nameplate.h"

ANPC_Base::ANPC_Base()
{
    PrimaryActorTick.bCanEverTick = false;
    NPCName = TEXT("Default NPC");
    NPCTypeTitle = TEXT("");

    InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
    InteractionSphere->SetupAttachment(GetRootComponent());
    InteractionSphere->InitSphereRadius(200.f);

    NameplateComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("Nameplate"));
    NameplateComponent->SetupAttachment(GetMesh());
    NameplateComponent->SetWidgetSpace(EWidgetSpace::Screen);
    NameplateComponent->SetRelativeLocation(FVector(0.f, 0.f, 200.f));

    static ConstructorHelpers::FClassFinder<UUserWidget> NameplateWidgetClass(TEXT("WidgetBlueprint'/Game/UE_Asset/UI/WBP_UI_NPC_Nameplate.WBP_UI_NPC_Nameplate_C'"));

    if (NameplateWidgetClass.Succeeded())
        NameplateComponent->SetWidgetClass(NameplateWidgetClass.Class);
}

void ANPC_Base::BeginPlay()
{
    Super::BeginPlay();

    InteractionSphere->SetCollisionProfileName(TEXT("Trigger"));
    InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ANPC_Base::OnOverlapBegin);
    InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ANPC_Base::OnOverlapEnd);

    UUI_NPC_Nameplate* NameplateWidget = Cast<UUI_NPC_Nameplate>(NameplateComponent->GetUserWidgetObject());
    if (IsValid(NameplateWidget))
        NameplateWidget->SetNameplate(NPCName, NPCTypeTitle);
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