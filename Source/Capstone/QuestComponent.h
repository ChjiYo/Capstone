#pragma once

#include "GameInfo.h"
#include "Components/ActorComponent.h"
#include "QuestComponent.generated.h"

class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnQuestTrackerChanged);

// 퀘스트 완료 때 지급되는 아이템 한 종류 (ResolveRewardItems의 결과)
struct FQuestRewardItem
{
	FName ItemID;
	int32 Count = 1;
};

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

	// 퀘스트 완료(보상 지급) 시 재생할 효과음. BP_PyGrey의 QuestComponent에서 지정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Sound")
	TObjectPtr<USoundBase> QuestCompleteSound;

	// 활성 퀘스트가 진행돼야 하는 지점의 액터.
	// 처치: 가장 가까운 생존 개체(없으면 해당 몬스터의 스폰포인트) / 채집: 가장 가까운 채집물 / 강화: 강화 NPC.
	// 그 목표가 이 레벨에 없고 다른 레벨에 있으면 그 레벨로 가는 APortalPoint를 돌려준다 (도착하면 실제 목표로 바뀐다).
	// 미니맵 마커와 월드 웨이포인트가 공용으로 쓴다. 결과를 캐시하고 TargetRefreshInterval마다만 다시 뒤지므로 매 Tick 불러도 된다.
	// 활성 퀘스트가 없거나, 목표가 없거나(대화형), 이미 완료됐으면 nullptr
	AActor* GetQuestTargetActor();

	// 퀘스트를 완료했을 때 실제로 지급되는 아이템 목록. 퀘스트 데이터(블루프린트)의 Reward 아이템에 코드가 보충하는 아이템을 더한 것이라,
	// 보상을 보여주는 UI도 Quest.Reward를 그대로 읽지 말고 이 값을 써야 실제 지급과 어긋나지 않는다 (경험치/골드는 Quest.Reward 그대로)
	static TArray<FQuestRewardItem> ResolveRewardItems(const FQuestData& Quest);

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

	// FindQuestTargetActor가 이 레벨에서 목표를 못 찾으면, 목표가 있는 레벨로 가는 포탈(QuestRoute)을 대신 돌려준다
	AActor* FindQuestTargetOrRoute(const FQuestData& Quest) const;

	// 현재 목표 지점으로 판단된 액터. 목표가 바뀌거나 대상이 사라지면 곧바로, 그 외에는 TargetRefreshInterval마다 다시 고른다
	TWeakObjectPtr<AActor> CachedTargetActor;
	EQuestObjectiveType CachedObjectiveType = EQuestObjectiveType::None;
	FName CachedTargetID = NAME_None;

	// 처치/채집 목표는 "가장 가까운 개체"가 플레이어가 움직이면 바뀌므로, 처음 고른 대상에 고정되지 않게 이 간격(초)마다 재탐색한다.
	// 마커(미니맵/웨이포인트)가 매 Tick 부르는 값이라 월드 전체를 매번 뒤지지 않도록 간격을 둔다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest", meta = (ClampMin = "0.05"))
	float TargetRefreshInterval = 0.3f;

	// 이 시각(월드 시간) 전에는 캐시된 대상을 그대로 쓴다
	double NextTargetRefreshTime = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
	FQuestData ActiveQuest;

	bool bHasActiveQuest = false;

	FDelegateHandle MonsterKilledHandle;
	FDelegateHandle ItemGatheredHandle;
	FDelegateHandle WeaponEnhancedHandle;
};
