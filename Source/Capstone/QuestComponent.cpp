#include "QuestComponent.h"
#include "Monster.h"
#include "GatherableObject.h"
#include "Py_Grey.h"
#include "InventoryComponent.h"
#include "ExperienceComponent.h"
#include "QuestStateSubsystem.h"
#include "SpawnPoint.h"
#include "ItemBase.h"
#include "EquipmentData.h"
#include "NPC_Enhance.h"
#include "QuestRoute.h"
#include "PortalPoint.h"
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

	// 퀘스트/목표 종류가 바뀌었거나 캐시해둔 대상이 사라졌으면 즉시 다시 탐색한다.
	// 그 외에도 처치/채집은 "가장 가까운 개체"가 플레이어의 이동에 따라 바뀌므로 일정 간격마다 다시 고른다
	// (한 번 정한 캐시를 계속 쓰던 예전 방식은, 처음 잡힌 먼 몬스터에 마커가 고정돼 바로 옆 몬스터를 무시하는 문제가 있었다).
	// 강화는 대상이 고정된 NPC라 재탐색이 필요 없다
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	const bool bTargetChanged = CachedObjectiveType != ActiveQuest.ObjectiveType || CachedTargetID != ActiveQuest.TargetID;
	const bool bRoamingTarget = ActiveQuest.ObjectiveType != EQuestObjectiveType::Enhance;

	// 대상이 없어서 계속 못 찾는 중(예: 목표 레벨로 가는 길도 없음)이면 매 Tick 월드를 뒤지지 않고 같은 간격으로만 다시 찾는다.
	// 반대로 잡고 있던 대상이 파괴됐으면(채집물을 주움 등) 간격과 무관하게 바로 다음 대상을 고른다
	const bool bTargetLost = CachedTargetActor.IsStale();
	const bool bRefreshDue = Now >= NextTargetRefreshTime;

	if (bTargetChanged || bTargetLost || (bRefreshDue && (bRoamingTarget || !CachedTargetActor.IsValid())))
	{
		CachedObjectiveType = ActiveQuest.ObjectiveType;
		CachedTargetID = ActiveQuest.TargetID;
		CachedTargetActor = FindQuestTargetOrRoute(ActiveQuest);
		NextTargetRefreshTime = Now + TargetRefreshInterval;
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
			// 이미 쓰러져 사라지는 중인 개체는 더 잡을 수 없으므로 목표에서 뺀다
			if (!IsValid(Monster) || Monster->MonsterID != Quest.TargetID || Monster->GetCurrentHp() <= 0)
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

AActor* UQuestComponent::FindQuestTargetOrRoute(const FQuestData& Quest) const
{
	if (AActor* Target = FindQuestTargetActor(Quest))
		return Target;

	// 이 레벨에 목표가 없다: 목표가 있는 레벨로 가는 포탈을 대신 가리킨다 (목표 레벨에 도착하면 위에서 실제 목표가 잡힌다)
	const AActor* Owner = GetOwner();
	return QuestRoute::FindRoutePortal(GetWorld(), Owner ? Owner->GetActorLocation() : FVector::ZeroVector, Quest);
}

TArray<FQuestRewardItem> UQuestComponent::ResolveRewardItems(const FQuestData& Quest)
{
	TArray<FQuestRewardItem> Items;

	if (!Quest.Reward.RewardItemID.IsNone() && Quest.Reward.RewardItemCount > 0)
		Items.Add({ Quest.Reward.RewardItemID, Quest.Reward.RewardItemCount });

	// 고블린을 4마리 잡는 '고블린 토벌'(BP_Hans)은 블루프린트에 다른 보상 아이템(HpPotion)이 이미 들어 있으므로,
	// 그 값을 덮어쓰지 않고 고블린 소드를 함께 얹는다. 블루프린트가 이미 소드를 주고 있으면 중복해서 주지 않는다
	if (Quest.ObjectiveType == EQuestObjectiveType::Kill && Quest.TargetID == FName(TEXT("Goblin"))
		&& !Items.ContainsByPredicate([](const FQuestRewardItem& Item) { return Item.ItemID == EquipDefs::GoblinSword; }))
	{
		Items.Add({ EquipDefs::GoblinSword, 1 });
	}

	return Items;
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

	const FQuestReward& Reward = ActiveQuest.Reward;

	UInventoryComponent* Inventory = Owner->FindComponentByClass<UInventoryComponent>();

	if (Inventory)
	{
		for (const FQuestRewardItem& Item : ResolveRewardItems(ActiveQuest))
		{
			Inventory->AddItem(Item.ItemID, Item.Count);
		}
	}

	if (Inventory && Reward.RewardGold > 0)
	{
		Inventory->AddGold(Reward.RewardGold);
	}

	if (Reward.RewardExp > 0)
	{
		if (UExperienceComponent* Experience = Owner->FindComponentByClass<UExperienceComponent>())
		{
			Experience->AddExp(Reward.RewardExp);
		}
	}
}
