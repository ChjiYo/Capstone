#include "StoryTrigger.h"
#include "StoryComponent.h"
#include "Py_Grey.h"
#include "NPC_Quest.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

AStoryTrigger::AStoryTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;
	TriggerBox->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
}

void AStoryTrigger::BeginPlay()
{
	Super::BeginPlay();

	// 오버랩 바인딩은 BeginPlay에서 1회만 (APortalPoint에서 Tick 바인딩으로 겪었던 중복 등록 버그 재발 방지)
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AStoryTrigger::OnOverlapBegin);

	if (TriggerMode == EStoryTriggerMode::LevelStart)
	{
		GetWorldTimerManager().SetTimer(StartTimerHandle, this, &AStoryTrigger::TryPlayWithLocalPlayer, StartDelay, false);
	}
}

void AStoryTrigger::TryPlayWithLocalPlayer()
{
	TryPlay(UGameplayStatics::GetPlayerPawn(this, 0));
}

void AStoryTrigger::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (TriggerMode != EStoryTriggerMode::Overlap)
		return;

	TryPlay(OtherActor);
}

void AStoryTrigger::TryPlay(AActor* PlayerActor)
{
	APy_Grey* Player = Cast<APy_Grey>(PlayerActor);
	if (!Player || !Player->StoryComponent || DialogueLines.Num() == 0)
		return;

	// QuestGiver가 지정돼 있으면 대사가 끝나는 즉시 수락/거절 UI 없이 현재 체인 퀘스트를 지급한다
	FSimpleDelegate OnFinished;
	if (QuestGiver)
	{
		OnFinished = FSimpleDelegate::CreateUObject(QuestGiver, &ANPC_Quest::GrantCurrentQuestFor, static_cast<AActor*>(Player));
	}

	bool bStarted = false;
	if (!BeatID.IsNone())
	{
		bStarted = Player->StoryComponent->PlayDialogueOnce(BeatID, DialogueLines, bLockInput, OnFinished);
	}
	else
	{
		bStarted = Player->StoryComponent->PlayDialogue(DialogueLines, bLockInput, OnFinished);
	}

	if (bStarted && bDestroyAfterPlay)
	{
		Destroy();
	}
}
