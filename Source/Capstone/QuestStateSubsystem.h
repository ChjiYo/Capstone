#pragma once

#include "GameInfo.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "QuestStateSubsystem.generated.h"

// OpenLevel로 레벨을 이동하면 APy_Grey(및 그 QuestComponent)가 통째로 파괴되고 새로 스폰되므로,
// 진행 중이던 퀘스트가 사라지지 않도록 게임 인스턴스에 활성 퀘스트 상태를 잠깐 보관해 둔다.
// UQuestComponent가 상태를 바꿀 때마다 여기에 저장하고, BeginPlay에서 다시 읽어온다.
UCLASS()
class CAPSTONE_API UQuestStateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void SaveActiveQuest(bool bInHasActiveQuest, const FQuestData& InQuest)
	{
		bHasActiveQuest = bInHasActiveQuest;
		ActiveQuest = InQuest;
	}

	// 저장된 퀘스트가 있으면 OutQuest에 채우고 true를 반환한다
	bool LoadActiveQuest(FQuestData& OutQuest) const
	{
		if (!bHasActiveQuest)
			return false;

		OutQuest = ActiveQuest;
		return true;
	}

private:
	bool bHasActiveQuest = false;
	FQuestData ActiveQuest;
};
