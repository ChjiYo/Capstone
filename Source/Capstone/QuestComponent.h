#pragma once

#include "GameInfo.h"
#include "Components/ActorComponent.h"
#include "QuestComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnQuestTrackerChanged);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CAPSTONE_API UQuestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UQuestComponent();

	// 퀘스트 수락 시 NPC_Quest가 호출. 처치 목표가 있으면 완료될 때까지 진행도를 추적한다
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void AcceptQuest(const FQuestData& InQuestData);

	UFUNCTION(BlueprintPure, Category = "Quest")
	bool HasActiveQuest() const { return bHasActiveQuest; }

	UFUNCTION(BlueprintPure, Category = "Quest")
	const FQuestData& GetActiveQuest() const { return ActiveQuest; }

	// 좌측 퀘스트 트래커 UI가 이 델리게이트를 구독해서 갱신
	UPROPERTY(BlueprintAssignable, Category = "Quest")
	FOnQuestTrackerChanged OnQuestTrackerChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void HandleMonsterKilled(FName MonsterID, int32 ExpReward, int32 GoldReward);
	void HandleItemGathered(FName ItemID, int32 Count);
	void HandleWeaponEnhanced(int32 NewLevel);

	// 목표 타입이 일치하고(+ Kill/Gather는 TargetID까지 일치) 진행 중이면 CurrentCount를 Amount만큼 올린다.
	// TargetCount에 도달하면 즉시 완료 처리 + 보상 지급.
	void TryAdvanceObjective(EQuestObjectiveType Type, FName InTargetID, int32 Amount);

	void GrantReward();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
	FQuestData ActiveQuest;

	bool bHasActiveQuest = false;

	FDelegateHandle MonsterKilledHandle;
	FDelegateHandle ItemGatheredHandle;
	FDelegateHandle WeaponEnhancedHandle;
};
