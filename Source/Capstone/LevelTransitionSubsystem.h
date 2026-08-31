#pragma once

#include "GameInfo.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LevelTransitionSubsystem.generated.h"

// 레벨 전환 한 번 분량의 "어디서 스폰할지" 정보를 게임 인스턴스에 잠깐 들고 있는 서브시스템.
// APortalPoint가 OpenLevel 직전에 태그를 세팅해두면, 새 레벨의 AGM_Main::ChoosePlayerStart_Implementation이
// 이걸 소비(Consume)해서 해당 태그를 가진 PlayerStart를 골라 쓴다.
// 게임을 처음 실행했을 때(포탈을 거치지 않았을 때)는 태그가 비어 있으므로 엔진 기본 동작(PlayerStart 아무거나/랜덤)으로 폴백된다.
UCLASS()
class CAPSTONE_API ULevelTransitionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void SetPendingPlayerStartTag(FName Tag) { PendingPlayerStartTag = Tag; }

	// 한 번 읽으면 비워짐 — 같은 레벨에 다시 들어갈 때(재시작 등) 이전 태그가 남아있지 않도록
	FName ConsumePendingPlayerStartTag()
	{
		const FName Tag = PendingPlayerStartTag;
		PendingPlayerStartTag = NAME_None;
		return Tag;
	}

private:
	FName PendingPlayerStartTag = NAME_None;
};
