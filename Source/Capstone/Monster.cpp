#include "Monster.h"
#include "Components/WidgetComponent.h"
#include "UI_HpBar.h"

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

void AMonster::SetAttackEnd()
{
	if (OnAttackEnd.IsBound())
	{
		OnAttackEnd.Broadcast();
	}
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

	if (IsValid(HpBarWidget))
	{
		HpBarWidget->SetHp(m_tInfo.iHp / static_cast<float>(m_tInfo.iMaxHp));
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
