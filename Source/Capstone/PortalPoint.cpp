#include "PortalPoint.h"
#include "Py_Grey.h"
#include "EffectDestroy.h"
#include "QuestComponent.h"
#include "InventoryComponent.h"
#include "ExperienceComponent.h"
#include "LevelTransitionSubsystem.h"
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

	if (ULevelTransitionSubsystem* TransitionSubsystem = GetGameInstance()->GetSubsystem<ULevelTransitionSubsystem>())
	{
		if (!TargetPlayerStartTag.IsNone())
		{
			TransitionSubsystem->SetPendingPlayerStartTag(TargetPlayerStartTag);
		}

		// OpenLevel(하드 레벨 전환)은 플레이어 폰까지 파괴하므로, 폰이 들고 있던 인벤토리(아이템/골드)를
		// 게임 인스턴스 서브시스템에 잠깐 맡겨뒀다가 새 레벨의 새 폰이 BeginPlay에서 복원해간다
		if (Player->InventoryComponent)
		{
			TransitionSubsystem->SetPendingInventory(Player->InventoryComponent->Items, Player->InventoryComponent->GetGold());
		}

		// 인벤토리와 마찬가지로 레벨/경험치도 폰과 함께 파괴되므로 넘겨서 새 폰이 복원하게 한다
		if (Player->ExperienceComponent)
		{
			TransitionSubsystem->SetPendingExperience(Player->ExperienceComponent->GetCurrentLevel(), Player->ExperienceComponent->GetCurrentExp());
		}

		// 체력도 폰과 함께 파괴되므로 넘겨서 새 폰이 복원하게 한다 (그렇지 않으면 레벨 이동마다 풀피로 초기화됨)
		TransitionSubsystem->SetPendingHp(Player->GetMaxHp(), Player->GetCurrentHp());
	}

	UGameplayStatics::OpenLevel(
		GetWorld(),
		TargetLevelName);
}

