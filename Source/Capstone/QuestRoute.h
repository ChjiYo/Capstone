#pragma once

#include "GameInfo.h"

class APortalPoint;

// 퀘스트 목표가 "다른 레벨"에 있을 때 거기까지 가는 길(포탈)을 안내하기 위한 도우미.
// 미니맵 마커/월드 웨이포인트가 쓰는 UQuestComponent::GetQuestTargetActor가, 현재 레벨에서 목표를 못 찾았을 때만 부른다.
namespace QuestRoute
{
	// 이 퀘스트의 목표(몬스터/채집물/NPC)가 있는 레벨 후보들.
	// FQuestData::TargetLevelName이 지정돼 있으면 그것 하나만, 비어 있으면 코드에 있는 표(목표 종류 + TargetID)에서 찾는다.
	// 표에도 없으면 빈 배열 (길 안내를 하지 않는다)
	TArray<FName> GetQuestLevels(const FQuestData& Quest);

	// 지금 레벨에서 퀘스트 목표 레벨 쪽으로 가려면 밟아야 하는 포탈. 갈 수 없으면(길이 없거나 게이트가 잠김) nullptr.
	// 첫 걸음은 지금 레벨에 실제로 놓인 포탈로 판단하고(게이트 RequiredQuestID 포함), 그 뒤 여러 레벨을 건너야 하면 코드의 레벨 연결표를 쓴다
	APortalPoint* FindRoutePortal(const UWorld* World, const FVector& FromLocation, const FQuestData& Quest);

	// "Lv_OpenField" 같은 현재 레벨 이름 (에디터 PIE 접두사 제거)
	FName GetCurrentLevelName(const UWorld* World);
}
