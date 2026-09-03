#include "Monster.h"
#include "MonsterAnim.h"
#include "Components/WidgetComponent.h"
#include "UI_HpBar.h"
#include "ItemBase.h"
#include "Materials/MaterialInstanceDynamic.h"

FOnMonsterKilled AMonster::OnMonsterKilled;
FOnBossDefeated AMonster::OnBossDefeated;

AMonster::AMonster()
{
	PrimaryActorTick.bCanEverTick = true;
	pSpawnPoint = nullptr;

	fTraceRange = 1000.f;
	fAttackDist = 150.f;

	HpBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HpBar"));
	HpBarComponent->SetupAttachment(GetMesh());
	HpBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HpBarComponent->SetRelativeLocation(FVector(0.f, 0.f, 100.f));

	static ConstructorHelpers::FClassFinder<UUserWidget> HpBarWidgetClass(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/UE_Asset/UI/BP_UI_Mon_HpBar.BP_UI_Mon_HpBar_C'"));

	if (HpBarWidgetClass.Succeeded())
		HpBarComponent->SetWidgetClass(HpBarWidgetClass.Class);

	bHit = false;

}

AMonster::~AMonster()
{
	if (pSpawnPoint)
		pSpawnPoint->Respawn();

	OnAttackEnd.Clear();
	AttackEndHandle.Reset(0);
}

void AMonster::ClearAttackEndDelegate()
{
	OnAttackEnd.Clear();
}

void AMonster::BeginPlay()
{
	Super::BeginPlay();

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Monster"));
	GetCapsuleComponent()->SetNotifyRigidBodyCollision(true);
	

	HpBarWidget = Cast<UUI_HpBar>(HpBarComponent->GetUserWidgetObject());

	if (IsValid(HpBarWidget))
		HpBarWidget->SetHp(m_tInfo.iHp / static_cast<float>(m_tInfo.iMaxHp));

	if (USkeletalMeshComponent* MonsterMesh = GetMesh())
	{
		HitFlashMaterials.Reset();

		const int32 NumMaterials = MonsterMesh->GetNumMaterials();
		for (int32 i = 0; i < NumMaterials; ++i)
		{
			if (UMaterialInstanceDynamic* DynMat = MonsterMesh->CreateAndSetMaterialInstanceDynamic(i))
			{
				HitFlashMaterials.Add(DynMat);
			}
		}
	}
}

void AMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateHitFlash(DeltaTime);
}

void AMonster::UpdateHitFlash(float DeltaTime)
{
	if (HitFlashTimeRemaining <= 0.f)
		return;

	HitFlashTimeRemaining = FMath::Max(0.f, HitFlashTimeRemaining - DeltaTime);

	const float FlashAmount = HitFlashDuration > 0.f ? HitFlashTimeRemaining / HitFlashDuration : 0.f;

	for (UMaterialInstanceDynamic* DynMat : HitFlashMaterials)
	{
		if (IsValid(DynMat))
			DynMat->SetScalarParameterValue(HitFlashParamName, FlashAmount);
	}
}

float AMonster::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    m_tInfo.iHp -= ActualDamage;

	Hit();

    if (IsValid(HpBarWidget))
    {
        HpBarWidget->SetHp(m_tInfo.iHp / static_cast<float>(m_tInfo.iMaxHp));
    }

    if (m_tInfo.iHp <= 0)
    {
        m_tInfo.iHp = 0;

        OnMonsterKilled.Broadcast(MonsterID, ExpReward, GoldReward);

        if (bIsBoss)
        {
            OnBossDefeated.Broadcast(MonsterID);
        }

            if (DropItemClasses.Num() > 0 && FMath::FRand() <= DropChance)
            {
                int32 RandomIndex = FMath::RandRange(0, DropItemClasses.Num() - 1);
                TSubclassOf<AItemBase> ItemToDrop = DropItemClasses[RandomIndex];

                if (ItemToDrop)
                {
                    FVector SpawnLocation = GetActorLocation() + FVector({ 0.f,-10.f , 0.f });

                    FRotator SpawnRotation = FRotator::ZeroRotator;

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

void AMonster::Hit()
{
	UMonsterAnim* pMonAnim = Cast<UMonsterAnim >(GetMesh()->GetAnimInstance());

	bHit = true;
	HitFlashTimeRemaining = HitFlashDuration;

	// bHit은 UMonsterAnim이 Hit 몽타주의 실제 종료 시점(OnHitMontageEnded)에 맞춰 false로 되돌린다
	if (IsValid(pMonAnim))
		pMonAnim->Hit();
}
