#include "PortalPoint.h"
#include "Py_Grey.h"
#include "EffectDestroy.h"
#include "QuestComponent.h"
#include "Kismet/GameplayStatics.h"

APortalPoint::APortalPoint()
{
	PrimaryActorTick.bCanEverTick = true;

	m_pCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	RootComponent = m_pCollision;

	m_pNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraEffect"));
	m_pNiagara->SetupAttachment(RootComponent);
	m_pNiagara->bAutoActivate = true;

}

void APortalPoint::BeginPlay()
{
	Super::BeginPlay();

	m_pCollision->OnComponentBeginOverlap.AddDynamic(this, &APortalPoint::OnOverlapBegin);

	if (NiagaraTemplate)
	{
		m_pNiagara->SetAsset(NiagaraTemplate);
	}
	m_pNiagara->Activate(true);

}

void APortalPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APortalPoint::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APy_Grey* Player = Cast<APy_Grey>(OtherActor);
	if (!Player)
		return;

	// 8단계 보스 맵 게이트: 지정된 퀘스트를 진행 중이어야만 통과 (None이면 기존처럼 무조건 통과)
	if (!RequiredQuestID.IsNone())
	{
		UQuestComponent* QC = Player->QuestComponent;

		if (!QC || !QC->HasActiveQuest() || QC->GetActiveQuest().QuestID != RequiredQuestID)
		{
			PrintViewport(2.f, FColor::Red, TEXT("아직은 아니다. 하란에게 먼저 얘기하자."));
			return;
		}
	}

	UGameplayStatics::OpenLevel(
		GetWorld(),
		TargetLevelName);
}

