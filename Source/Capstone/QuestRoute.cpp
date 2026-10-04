#include "QuestRoute.h"
#include "PortalPoint.h"
#include "EngineUtils.h"

namespace
{
	// 목표 종류 + TargetID가 어느 레벨에 있는지 (레벨에 배치된 몬스터/채집물/NPC를 실제로 덤프해서 만든 표).
	// 새 퀘스트 목표가 생기면 여기에 한 줄 추가하거나, 퀘스트 데이터의 TargetLevelName을 채우면 된다
	struct FQuestLevelRow
	{
		EQuestObjectiveType Type;
		const TCHAR* TargetID;      // nullptr이면 TargetID를 보지 않는다 (강화)
		TArray<const TCHAR*> Levels;
	};

	const TArray<FQuestLevelRow>& QuestLevelTable()
	{
		static const TArray<FQuestLevelRow> Table = {
			{ EQuestObjectiveType::Gather,  TEXT("Herb"),    { TEXT("Lv_OpenField") } },
			{ EQuestObjectiveType::Kill,    TEXT("Goblin"),  { TEXT("Lv_OpenField"), TEXT("LV_Cave"), TEXT("Lv_DefaultField") } },
			{ EQuestObjectiveType::Kill,    TEXT("Narbash"), { TEXT("LV_Cave"), TEXT("Lv_Boss") } },
			{ EQuestObjectiveType::Enhance, nullptr,         { TEXT("Lv_Village") } },
		};
		return Table;
	}

	// 레벨 -> 그 레벨의 포탈이 보내주는 레벨들. 지금 레벨은 실제 포탈로 덮어쓰므로 "다른 레벨에서 몇 번 더 건너야 하는가"를 셀 때만 쓰인다
	const TMap<FName, TArray<FName>>& StaticLevelLinks()
	{
		static const TMap<FName, TArray<FName>> Links = {
			{ TEXT("Lv_Village"),      { TEXT("Lv_OpenField") } },
			{ TEXT("Lv_OpenField"),    { TEXT("Lv_Village"), TEXT("LV_Cave") } },
			{ TEXT("Lv_DefaultField"), { TEXT("Lv_Boss") } },
		};
		return Links;
	}

	// 게이트 포탈은 그 퀘스트를 진행 중일 때만 지나갈 수 있다
	bool CanPass(const APortalPoint* Portal, const FQuestData& Quest)
	{
		return Portal->GetRequiredQuestID().IsNone() || Portal->GetRequiredQuestID() == Quest.QuestID;
	}
}

namespace QuestRoute
{
	FName GetCurrentLevelName(const UWorld* World)
	{
		return World ? FName(*UWorld::RemovePIEPrefix(World->GetMapName())) : NAME_None;
	}

	TArray<FName> GetQuestLevels(const FQuestData& Quest)
	{
		TArray<FName> Result;

		if (!Quest.TargetLevelName.IsNone())
		{
			Result.Add(Quest.TargetLevelName);
			return Result;
		}

		for (const FQuestLevelRow& Row : QuestLevelTable())
		{
			if (Row.Type != Quest.ObjectiveType)
				continue;
			if (Row.TargetID && Quest.TargetID != FName(Row.TargetID))
				continue;

			for (const TCHAR* Level : Row.Levels)
				Result.Add(FName(Level));
			break;
		}
		return Result;
	}

	APortalPoint* FindRoutePortal(const UWorld* World, const FVector& FromLocation, const FQuestData& Quest)
	{
		if (!World)
			return nullptr;

		const TArray<FName> Dests = GetQuestLevels(Quest);
		if (Dests.IsEmpty())
			return nullptr;

		const FName Current = GetCurrentLevelName(World);
		if (Dests.Contains(Current))
			return nullptr; // 목표 레벨에 이미 와 있다. 여기서 못 찾은 건 길 문제가 아니라 목표물이 (잠시) 없는 것

		// 지금 레벨에서 나갈 수 있는 포탈들: 보내주는 레벨 -> 그 레벨로 가는 포탈(가장 가까운 것)
		TMap<FName, APortalPoint*> ExitPortals;
		for (TActorIterator<APortalPoint> It(World); It; ++It)
		{
			APortalPoint* Portal = *It;
			if (!IsValid(Portal) || Portal->GetTargetLevelName().IsNone() || !CanPass(Portal, Quest))
				continue;

			APortalPoint*& Best = ExitPortals.FindOrAdd(Portal->GetTargetLevelName());
			if (!Best || FVector::DistSquared(FromLocation, Portal->GetActorLocation()) < FVector::DistSquared(FromLocation, Best->GetActorLocation()))
				Best = Portal;
		}

		// 너비 우선 탐색: 첫 걸음(=지금 레벨의 어느 포탈로 나가는가)별로 목표 레벨까지 걸리는 걸음 수를 센다.
		// 큐에는 (레벨, 첫 걸음으로 택한 레벨)을 넣고, 처음 닿는 목표 레벨의 첫 걸음이 최단 경로의 첫 걸음이다
		TArray<TPair<FName, FName>> Queue;
		TSet<FName> Visited;
		Visited.Add(Current);
		for (const TPair<FName, APortalPoint*>& Exit : ExitPortals)
		{
			Queue.Add({ Exit.Key, Exit.Key });
			Visited.Add(Exit.Key);
		}

		const TMap<FName, TArray<FName>>& Links = StaticLevelLinks();
		for (int32 Head = 0; Head < Queue.Num(); ++Head)
		{
			const FName Level = Queue[Head].Key;
			const FName FirstHop = Queue[Head].Value;

			if (Dests.Contains(Level))
				return ExitPortals.FindRef(FirstHop);

			if (const TArray<FName>* Next = Links.Find(Level))
			{
				for (const FName& NextLevel : *Next)
				{
					if (!Visited.Contains(NextLevel))
					{
						Visited.Add(NextLevel);
						Queue.Add({ NextLevel, FirstHop });
					}
				}
			}
		}

		return nullptr;
	}
}
