#include "ItemBase.h"
#include "Components/SphereComponent.h"
#include "Components/BillboardComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "InventoryComponent.h"
#include "Py_Grey.h"
#include "DrawDebugHelpers.h"
#include "CapstoneSFX.h"
#include "EquipmentData.h"

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

    // 전리품 아이콘. 텍스처는 BeginPlay에서 아이템 ID를 보고 채우고, 그때까지(그리고 bShowIconSprite가 꺼진 픽업은 계속) 숨겨 둔다
    IconSprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("IconSprite"));
    IconSprite->SetupAttachment(RootComponent);
    IconSprite->SetHiddenInGame(true);
    IconSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    IconSprite->bIsScreenSizeScaled = false;
    IconSprite->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
    IconSprite->SetRelativeScale3D(FVector(0.9f));

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

    if (bShowIconSprite && IconSprite)
    {
        if (UTexture2D* Icon = EquipDefs::GetGeneratedIcon(ItemID))
        {
            IconSprite->SetSprite(Icon);
            IconSprite->SetHiddenInGame(false);
        }
    }
}

AItemBase* AItemBase::SpawnLoot(AActor* Dropper, FName InItemID, int32 InCount, const FVector& ScatterOffset)
{
    UWorld* World = Dropper ? Dropper->GetWorld() : nullptr;

    if (!World || InItemID.IsNone() || InCount <= 0)
        return nullptr;

    // 죽어가는 몬스터의 캡슐 한가운데가 아니라 바닥 위에 놓는다
    FVector Location = Dropper->GetActorLocation() + ScatterOffset;

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(LootGroundTrace), false, Dropper);

    if (World->LineTraceSingleByChannel(Hit, Location, Location - FVector(0.f, 0.f, 600.f), ECC_Visibility, Params))
        Location = Hit.ImpactPoint + FVector(0.f, 0.f, 20.f);

    AItemBase* Loot = World->SpawnActorDeferred<AItemBase>(AItemBase::StaticClass(), FTransform(Location));

    if (!Loot)
        return nullptr;

    Loot->ItemID = InItemID;
    Loot->ItemType = EItemType::Loot;
    Loot->ItemCount = InCount;
    Loot->bShowIconSprite = true;

    Loot->FinishSpawning(FTransform(Location));
    return Loot;
}

void AItemBase::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    APy_Grey* Player = Cast<APy_Grey>(OtherActor);
    if (Player)
    {
        Player->SetInteractableItem(this);
        CapstoneSFX::Play2D(this, ESFX::Interact_Prompt);
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
                // 가방이 가득 차면 줍지 못하고 그 자리에 남겨 둔다 (아이템이 보이지 않는 칸으로 사라지지 않도록)
                if (!PlayerCharacter->InventoryComponent->CanAddItem(ItemID, ItemCount))
                {
                    CapstoneSFX::Play2D(this, ESFX::UI_Deny);
                    return;
                }

                PlayerCharacter->InventoryComponent->AddItem(ItemID, ItemCount);

                CapstoneSFX::Play2D(this, ESFX::UI_ItemPickup);

                Destroy();
            }
        }

    }
}