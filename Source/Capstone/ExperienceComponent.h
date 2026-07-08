#pragma once

#include "GameInfo.h"
#include "Components/ActorComponent.h"
#include "ExperienceComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnExpChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelUp, int32, NewLevel);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CAPSTONE_API UExperienceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UExperienceComponent();

	// 몬스터 처치/퀘스트 보상 등 외부에서 경험치를 지급할 때 호출. 레벨업 조건을 넘으면 여러 레벨도 한 번에 처리한다
	UFUNCTION(BlueprintCallable, Category = "Experience")
	void AddExp(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Experience")
	int32 GetCurrentLevel() const { return CurrentLevel; }

	UFUNCTION(BlueprintPure, Category = "Experience")
	int32 GetCurrentExp() const { return CurrentExp; }

	UFUNCTION(BlueprintPure, Category = "Experience")
	int32 GetRequiredExp() const { return RequiredExp; }

	// EXP 바 등 HUD 위젯이 구독해서 갱신
	UPROPERTY(BlueprintAssignable, Category = "Experience")
	FOnExpChanged OnExpChanged;

	// 레벨업 시 캐릭터의 스탯(체력/공격력)을 올리는 쪽(APy_Grey)이 구독
	UPROPERTY(BlueprintAssignable, Category = "Experience")
	FOnLevelUp OnLevelUp;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void HandleMonsterKilled(FName MonsterID, int32 ExpReward, int32 GoldReward);
	void RecalculateRequiredExp();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Experience")
	int32 CurrentLevel = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Experience")
	int32 CurrentExp = 0;

	// 레벨업에 필요한 경험치. CurrentLevel이 바뀔 때마다 BaseRequiredExp * ExpGrowthRate^(Level-1)로 재계산됨
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Experience")
	int32 RequiredExp = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Experience")
	int32 BaseRequiredExp = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Experience")
	float ExpGrowthRate = 1.15f;

	FDelegateHandle MonsterKilledHandle;
};
