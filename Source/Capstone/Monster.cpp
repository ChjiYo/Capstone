#include "Monster.h"
#include "Components/WidgetComponent.h"
#include "UI_HpBar.h"
#include "ItemBase.h"

AMonster::AMonster()
{
	PrimaryActorTick.bCanEverTick = true;
	pSpawnPoint = nullptr;

	fTraceRange = 1000.f;
	fAttackDist = 200.f;

	HpBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HpBar"));
	HpBarComponent->SetupAttachment(GetMesh());
	HpBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HpBarComponent->SetRelativeLocation(FVector(0.f, 0.f, 100.f));

	static ConstructorHelpers::FClassFinder<UUserWidget> HpBarWidgetClass(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/UE_Asset/UI/BP_UI_Mon_HpBar.BP_UI_Mon_HpBar_C'"));

	if (HpBarWidgetClass.Succeeded())
		HpBarComponent->SetWidgetClass(HpBarWidgetClass.Class);

}

AMonster::~AMonster()
{
	if (pSpawnPoint)
		pSpawnPoint->Respawn();

	OnAttackEnd.Clear();
	AttackEndHandle.Reset(0);
}

void AMonster::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{

}

void AMonster::BeginPlay()
{
	Super::BeginPlay();

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Monster"));
	GetCapsuleComponent()->SetNotifyRigidBodyCollision(true);
	

	HpBarWidget = Cast<UUI_HpBar>(HpBarComponent->GetUserWidgetObject());

	if (IsValid(HpBarWidget))
		HpBarWidget->SetHp(m_tInfo.iHp / static_cast<float>(m_tInfo.iMaxHp));
}

void AMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

float AMonster::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    // 1. 실제로 데미지 적용 (이 부분이 빠져있어서 추가했습니다!)
    m_tInfo.iHp -= ActualDamage;

    // 2. HP 바 UI 업데이트
    if (IsValid(HpBarWidget))
    {
        HpBarWidget->SetHp(m_tInfo.iHp / static_cast<float>(m_tInfo.iMaxHp));
    }

    // 3. 체력이 0 이하가 되어 죽었을 때의 로직
    if (m_tInfo.iHp <= 0)
    {
        // 이미 죽었으므로 HP를 0으로 고정 (음수 방지)
        m_tInfo.iHp = 0;

            // 드랍할 아이템 목록이 하나라도 등록되어 있는지 확인
            if (DropItemClasses.Num() > 0)
            {
                // 배열 안에서 랜덤한 인덱스 뽑기
                int32 RandomIndex = FMath::RandRange(0, DropItemClasses.Num() - 1);
                TSubclassOf<AItemBase> ItemToDrop = DropItemClasses[RandomIndex];

                if (ItemToDrop)
                {
                    // 몬스터의 현재 위치에서 아이템 스폰
                    FVector SpawnLocation = GetActorLocation();

                    FRotator SpawnRotation = FRotator::ZeroRotator;

                    // 월드에 아이템 생성
                    GetWorld()->SpawnActor<AItemBase>(ItemToDrop, SpawnLocation, SpawnRotation);

                    UE_LOG(LogTemp, Warning, TEXT("고블린이 아이템을 드랍했습니다!"));
                }
            }
            
        Destroy();
    }

   
    return ActualDamage;
}

void AMonster::AttackEnd()
{
	OnAttackEnd.Broadcast();

	for (FDelegateHandle& handle : AttackEndHandle)
	{
		OnAttackEnd.Remove(handle);
	}
}
