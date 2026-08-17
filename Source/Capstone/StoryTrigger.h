#pragma once

#include "GameInfo.h"
#include "GameFramework/Actor.h"
#include "StoryTrigger.generated.h"

UENUM(BlueprintType)
enum class EStoryTriggerMode : uint8
{
	Overlap,      // 플레이어가 볼륨에 들어오면
	LevelStart    // 레벨 시작 후 StartDelay초 뒤 (프롤로그용)
};

// 레벨에 배치해서 대사를 재생하는 트리거 액터 (11단계). 코드 변경 없이 레벨마다 배치만으로 연출 지점을 늘릴 수 있다
UCLASS()
class CAPSTONE_API AStoryTrigger : public AActor
{
	GENERATED_BODY()

public:
	AStoryTrigger();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Story")
	class UBoxComponent* TriggerBox;

	UPROPERTY(EditAnywhere, Category = "Story")
	EStoryTriggerMode TriggerMode = EStoryTriggerMode::Overlap;

	UPROPERTY(EditAnywhere, Category = "Story")
	TArray<FName> DialogueLines;

	// 비워두면 들어올 때마다 재생. 값을 넣으면 게임 인스턴스 수명 동안 1회만 재생
	UPROPERTY(EditAnywhere, Category = "Story")
	FName BeatID;

	UPROPERTY(EditAnywhere, Category = "Story")
	bool bLockInput = true;

	UPROPERTY(EditAnywhere, Category = "Story", meta = (EditCondition = "TriggerMode == EStoryTriggerMode::LevelStart"))
	float StartDelay = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Story")
	bool bDestroyAfterPlay = true;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void TryPlay(AActor* PlayerActor);

	// LevelStart 모드 타이머 콜백 (멤버 함수 오버로드라 FTimerDelegate/람다가 필요 없다)
	void TryPlayWithLocalPlayer();

private:
	FTimerHandle StartTimerHandle;
};
