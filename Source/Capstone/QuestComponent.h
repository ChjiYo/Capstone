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

	void HandleMonsterKilled(FName MonsterID);
	void GrantReward();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
	FQuestData ActiveQuest;

	bool bHasActiveQuest = false;

	FDelegateHandle MonsterKilledHandle;
};
