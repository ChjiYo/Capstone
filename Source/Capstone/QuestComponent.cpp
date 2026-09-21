#include "QuestComponent.h"
#include "Monster.h"
#include "GatherableObject.h"
#include "Py_Grey.h"
#include "InventoryComponent.h"
#include "ExperienceComponent.h"
#include "QuestStateSubsystem.h"
#include "SpawnPoint.h"
#include "ItemBase.h"
#include "NPC_Enhance.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "CapstoneSFX.h"

UQuestComponent::UQuestComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UQuestComponent::BeginPlay()
{
	Super::BeginPlay();

	MonsterKilledHandle = AMonster::OnMonsterKilled.AddUObject(this, &UQuestComponent::HandleMonsterKilled);
	ItemGatheredHandle = AGatherableObject::OnItemGathered.AddUObject(this, &UQuestComponent::HandleItemGathered);
	WeaponEnhancedHandle = APy_Grey::OnWeaponEnhanced.AddUObject(this, &UQuestComponent::HandleWeaponEnhanced);

	// 이전 레벨에서 진행 중이던 퀘스트가 있으면 복원한다 (UI 갱신은 소유 액터가 델리게이트를 바인딩한 뒤 직접 호출)
	if (UQuestStateSubsystem* QS = UGameplayStatics::GetGameInstance(this)->GetSubsystem<UQuestStateSubsystem>())
	{
		FQuestData Restored;
		if (QS->LoadActiveQuest(Restored))
		{
			ActiveQuest = Restored;
			bHasActiveQuest = true;
		}
	}
}

void UQuestComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AMonster::OnMonsterKilled.Remove(MonsterKilledHandle);
	AGatherableObject::OnItemGathered.Remove(ItemGatheredHandle);
	APy_Grey::OnWeaponEnhanced.Remove(WeaponEnhancedHandle);

	Super::EndPlay(EndPlayReason);
}

void UQuestComponent::AcceptQuest(const FQuestData& InQuestData)
{
	ActiveQuest = InQuestData;
	ActiveQuest.CurrentCount = 0;
	ActiveQuest.bIsCompleted = false;
	bHasActiveQuest = true;

	if (!ActiveQuest.HasObjective())
	{
		ActiveQuest.bIsCompleted = true;
		GrantReward();
	}

	SyncToStateSubsystem();
	OnQuestTrackerChanged.Broadcast();
}

void UQuestComponent::HandleMonsterKilled(FName MonsterID, int32 ExpReward, int32 GoldReward)
{
	TryAdvanceObjective(EQuestObjectiveType::Kill, MonsterID, 1);
}

void UQuestComponent::HandleItemGathered(FName ItemID, int32 Count)
{
	TryAdvanceObjective(EQuestObjectiveType::Gather, ItemID, Count);
}

void UQuestComponent::HandleWeaponEnhanced(int32 NewLevel)
{
	TryAdvanceObjective(EQuestObjectiveType::Enhance, NAME_None, 1);
}

void UQuestComponent::TryAdvanceObjective(EQuestObjectiveType Type, FName InTargetID, int32 Amount)
{
	if (!bHasActiveQuest || ActiveQuest.bIsCompleted)
		return;

	if (ActiveQuest.ObjectiveType != Type)
		return;

	// Kill/Gather는 대상 ID까지 일치해야 함. Enhance는 대상 ID 없이 횟수만 센다.
	if (Type != EQuestObjectiveType::Enhance)
	{
		if (ActiveQuest.TargetID.IsNone() || ActiveQuest.TargetID != InTargetID)
			return;
	}

	ActiveQuest.CurrentCount = FMath::Min(ActiveQuest.CurrentCount + Amount, ActiveQuest.TargetCount);

	if (ActiveQuest.CurrentCount >= ActiveQuest.TargetCount)
	{
		ActiveQuest.bIsCompleted = true;
		GrantReward();
	}
	else
	{
		CapstoneSFX::Play2D(this, ESFX::UI_QuestUpdate); // 완료 때는 완료음이 대신 울린다
	}

	SyncToStateSubsystem();
	OnQuestTrackerChanged.Broadcast();
}

void UQuestComponent::SyncToStateSubsystem()
{
	if (UQuestStateSubsystem* QS = UGameplayStatics::GetGameInstance(this)->GetSubsystem<UQuestStateSubsystem>())
	{
		QS->SaveActiveQuest(bHasActiveQuest, ActiveQuest);
	}
}

AActor* UQuestComponent::GetQuestTargetActor()
{
	// 처치/채집/강화 목표가 없거나(대화형 퀘스트) 이미 완료돼 보상까지 지급된 상태면 더 갈 곳이 없다
	if (!bHasActiveQuest || ActiveQuest.bIsCompleted || !ActiveQuest.HasObjective())
	{
		CachedTargetActor.Reset();
		return nullptr;
	}

	// 퀘스트/목표 종류가 바뀌었거나 캐시해둔 대상이 사라졌을 때만(몬스터 처치 등) 다시 월드를 탐색한다
	if (!CachedTargetActor.IsValid() || CachedObjectiveType != ActiveQuest.ObjectiveType || CachedTargetID != ActiveQuest.TargetID)
	{
		CachedObjectiveType = ActiveQuest.ObjectiveType;
		CachedTargetID = ActiveQuest.TargetID;
		CachedTargetActor = FindQuestTargetActor(ActiveQuest);
	}

	return CachedTargetActor.Get();
}

AActor* UQuestComponent::FindQuestTargetActor(const FQuestData& Quest) const
{
	UWorld* World = GetWorld();
	if (!World)
		return nullptr;

	const AActor* Owner = GetOwner();
	const FVector OwnerLoc = Owner ? Owner->GetActorLocation() : FVector::ZeroVector;

	switch (Quest.ObjectiveType)
	{
	case EQuestObjectiveType::Kill:
	{
		AActor* Nearest = nullptr;
		float BestDistSq = -1.f;
		for (TActorIterator<AMonster> It(World); It; ++It)
		{
			AMonster* Monster = *It;
			if (!IsValid(Monster) || Monster->MonsterID != Quest.TargetID)
				continue;

			const float DistSq = FVector::DistSquared(OwnerLoc, Monster->GetActorLocation());
			if (!Nearest || DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Nearest = Monster;
			}
		}

		if (Nearest)
			return Nearest;

		// 생존 개체가 하나도 없으면(리스폰 대기 등) 해당 몬스터를 스폰하는 스폰포인트를 대신 가리킨다
		for (TActorIterator<ASpawnPoint> It(World); It; ++It)
		{
			TSubclassOf<AMonster> SpawnType = It->GetSpawnType();
			if (!SpawnType)
				continue;

			const AMonster* DefaultMonster = SpawnType->GetDefaultObject<AMonster>();
			if (DefaultMonster && DefaultMonster->MonsterID == Quest.TargetID)
				return *It;
		}
		return nullptr;
	}
	case EQuestObjectiveType::Gather:
	{
		AActor* Nearest = nullptr;
		float BestDistSq = -1.f;
		for (TActorIterator<AItemBase> It(World); It; ++It)
		{
			AItemBase* Item = *It;
			if (!IsValid(Item) || Item->GetItemID() != Quest.TargetID)
				continue;

			const float DistSq = FVector::DistSquared(OwnerLoc, Item->GetActorLocation());
			if (!Nearest || DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Nearest = Item;
			}
		}
		return Nearest;
	}
	case EQuestObjectiveType::Enhance:
	{
		for (TActorIterator<ANPC_Enhance> It(World); It; ++It)
		{
			if (IsValid(*It))
				return *It;
		}
		return nullptr;
	}
	default:
		return nullptr;
	}
}

void UQuestComponent::GrantReward()
{
	AActor* Owner = GetOwner();
	if (!Owner)
		return;

	if (QuestCompleteSound)
	{
		UGameplayStatics::PlaySound2D(this, QuestCompleteSound);
	}
	else
	{
		// BP에 완료음이 지정돼 있지 않으면 기본 완료음(Quest_Complete)
		CapstoneSFX::Play2D(this, ESFX::UI_QuestComplete);
	}

	UInventoryComponent* Inventory = Owner->FindComponentByClass<UInventoryComponent>();

	if (Inventory && !ActiveQuest.Reward.RewardItemID.IsNone())
	{
		Inventory->AddItem(ActiveQuest.Reward.RewardItemID, ActiveQuest.Reward.RewardItemCount);
	}

	if (Inventory && ActiveQuest.Reward.RewardGold > 0)
	{
		Inventory->AddGold(ActiveQuest.Reward.RewardGold);
	}

	if (ActiveQuest.Reward.RewardExp > 0)
	{
		if (UExperienceComponent* Experience = Owner->FindComponentByClass<UExperienceComponent>())
		{
			Experience->AddExp(ActiveQuest.Reward.RewardExp);
		}
	}
}
