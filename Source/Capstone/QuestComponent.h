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

	// 활성 퀘스트가 진행돼야 하는 지점의 액터.
	// 처치: 가장 가까운 생존 개체(없으면 해당 몬스터의 스폰포인트) / 채집: 가장 가까운 채집물 / 강화: 강화 NPC.
	// 미니맵 마커와 월드 웨이포인트가 공용으로 쓴다. 결과를 캐시하므로 매 Tick 불러도 월드를 다시 뒤지지 않는다.
	// 활성 퀘스트가 없거나, 목표가 없거나(대화형), 이미 완료됐으면 nullptr
	AActor* GetQuestTargetActor();

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

	// 레벨 이동 후에도 진행 중인 퀘스트가 유지되도록 게임 인스턴스 서브시스템에 현재 상태를 저장
	void SyncToStateSubsystem();

	// ObjectiveType/TargetID에 맞는 월드 액터를 새로 탐색한다 (GetQuestTargetActor의 캐시 미스 시에만 호출)
	AActor* FindQuestTargetActor(const FQuestData& Quest) const;

	// 현재 목표 지점으로 판단된 액터. 대상이 사라지거나(몬스터 처치 등) 목표가 바뀌기 전까지는 재탐색하지 않는다
	TWeakObjectPtr<AActor> CachedTargetActor;
	EQuestObjectiveType CachedObjectiveType = EQuestObjectiveType::None;
	FName CachedTargetID = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
	FQuestData ActiveQuest;

	bool bHasActiveQuest = false;

	FDelegateHandle MonsterKilledHandle;
	FDelegateHandle ItemGatheredHandle;
	FDelegateHandle WeaponEnhancedHandle;
};
