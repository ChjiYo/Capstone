#pragma once

#include "GameInfo.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StoryStateSubsystem.generated.h"

// 프롤로그/보스 조우처럼 "한 번만" 재생돼야 하는 연출의 기록. 게임 인스턴스 수명 동안 유지된다
// (세이브 기능은 범위 밖 — 게임을 재실행하면 초기화된다. 9단계 "만들지 않는 것" 참고)
UCLASS()
class CAPSTONE_API UStoryStateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Story")
	bool HasSeen(FName BeatID) const { return !BeatID.IsNone() && SeenBeats.Contains(BeatID); }

	UFUNCTION(BlueprintCallable, Category = "Story")
	void MarkSeen(FName BeatID) { if (!BeatID.IsNone()) SeenBeats.Add(BeatID); }

	UFUNCTION(BlueprintCallable, Category = "Story")
	void SetChapter(int32 InChapter) { CurrentChapter = InChapter; }

	UFUNCTION(BlueprintPure, Category = "Story")
	int32 GetChapter() const { return CurrentChapter; }

private:
	TSet<FName> SeenBeats;
	int32 CurrentChapter = 0;
};
