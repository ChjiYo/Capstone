#pragma once

#include "EngineMinimal.h"
#include "EngineGlobals.h"
#include "DestructibleComponent.h"
#include "Engine.h"
#include "Engine/AssetManager.h"
#include "Runtime/Engine/Classes/Engine/GameEngine.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NavigationPath.h"

#include "GameInfo.generated.h" 

UENUM(BlueprintType)
enum class EDir : uint8
{
	Front,
	Back,
	Right,
	Left
};

USTRUCT(BlueprintType)
struct FQuestReward
{
    GENERATED_BODY()

    // InventoryComponent::AddItem / AItemBase::ItemID 와 동일한 값을 사용
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Reward")
    FName RewardItemID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Reward")
    int32 RewardItemCount = 1;
};

USTRUCT(BlueprintType)
struct FQuestData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FName QuestID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FText QuestTitle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FText QuestDescription;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FQuestReward Reward;

    // AMonster::MonsterID 와 동일한 값을 넣으면 처치 퀘스트가 됨. None이면 처치 목표 없는 퀘스트
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Objective")
    FName KillTargetID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Objective")
    int32 KillTargetCount = 0;

    // 진행도. 에디터에서 건드릴 값이 아니라 QuestComponent가 런타임에 갱신
    UPROPERTY(BlueprintReadOnly, Category = "Quest|Objective")
    int32 CurrentKillCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    bool bIsCompleted = false;

    bool HasKillObjective() const { return KillTargetCount > 0; }
};

DECLARE_LOG_CATEGORY_EXTERN(UE7, Log, All);
#define LOG_CALLINFO		(FString(__FUNCTION__) + TEXT("{ ") + FString::FromInt(__LINE__) + TEXT(" }")) 
#define LOG(Format, ...)		UE_LOG(UE7, Warning, TEXT("%s : %s"), *LOG_CALLINFO, *FString::Printf(Format, ##__VA_ARGS__))


inline void PrintViewport(float fTime, const FColor& Color, const FString& strText)
{
	GEngine->AddOnScreenDebugMessage(-1, fTime, Color, strText);
}