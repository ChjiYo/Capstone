#pragma once

#include "GameInfo.h"
#include "InventoryComponent.h"
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

	// 포탈로 OpenLevel 하기 직전, 파괴될 플레이어 폰의 인벤토리를 게임 인스턴스에 잠깐 보관해둔다.
	// OpenLevel(하드 레벨 전환)은 폰을 포함한 월드 전체를 파괴하고 새로 생성하므로,
	// 폰에 붙어있는 UInventoryComponent는 그대로는 새 레벨로 넘어가지 않는다.
	void SetPendingInventory(const TArray<FInventorySlot>& InItems, int32 InGold)
	{
		PendingInventoryItems = InItems;
		PendingInventoryGold = InGold;
		bHasPendingInventory = true;
	}

	// 한 번 읽으면 비워짐 — 인벤토리를 들고 있지 않은 최초 시작 시에는 false를 반환해 기본값(빈 인벤토리)을 유지
	bool ConsumePendingInventory(TArray<FInventorySlot>& OutItems, int32& OutGold)
	{
		if (!bHasPendingInventory)
			return false;

		OutItems = PendingInventoryItems;
		OutGold = PendingInventoryGold;
		bHasPendingInventory = false;
		return true;
	}

	// 포탈로 OpenLevel 하기 직전, 파괴될 플레이어 폰의 레벨/경험치를 게임 인스턴스에 잠깐 보관해둔다.
	// 인벤토리와 마찬가지로 UExperienceComponent도 폰이 파괴되면 같이 사라지므로 별도로 넘겨줘야 한다.
	void SetPendingExperience(int32 InLevel, int32 InExp)
	{
		PendingLevel = InLevel;
		PendingExp = InExp;
		bHasPendingExperience = true;
	}

	// 한 번 읽으면 비워짐 — 경험치를 들고 있지 않은 최초 시작 시에는 false를 반환해 기본값(레벨 1)을 유지
	bool ConsumePendingExperience(int32& OutLevel, int32& OutExp)
	{
		if (!bHasPendingExperience)
			return false;

		OutLevel = PendingLevel;
		OutExp = PendingExp;
		bHasPendingExperience = false;
		return true;
	}

	// 포탈로 OpenLevel 하기 직전, 파괴될 플레이어 폰의 체력을 게임 인스턴스에 잠깐 보관해둔다.
	// 인벤토리/경험치와 마찬가지로 폰이 파괴되면 체력도 같이 사라져 생성자의 기본값(풀피)으로
	// 되돌아가므로, 레벨 이동마다 체력이 초기화되는 것을 막기 위해 넘겨서 새 폰이 복원하게 한다.
	void SetPendingHp(int32 InMaxHp, int32 InCurrentHp)
	{
		PendingMaxHp = InMaxHp;
		PendingCurrentHp = InCurrentHp;
		bHasPendingHp = true;
	}

	// 한 번 읽으면 비워짐 — 체력을 들고 있지 않은 최초 시작 시에는 false를 반환해 기본값(풀피)을 유지
	bool ConsumePendingHp(int32& OutMaxHp, int32& OutCurrentHp)
	{
		if (!bHasPendingHp)
			return false;

		OutMaxHp = PendingMaxHp;
		OutCurrentHp = PendingCurrentHp;
		bHasPendingHp = false;
		return true;
	}

	// 포탈/리스폰으로 OpenLevel 하기 직전, 파괴될 플레이어 폰의 공격력과 무기 강화 단계를 보관해둔다.
	// 레벨업/무기 강화로 오른 공격력은 폰(m_tInfo)에만 들어 있어서, 안 넘기면 새 폰이 생성자의 기본 공격력으로
	// 되돌아간다. 체력과 달리 경험치 복원(RestoreExp)은 레벨업 이벤트를 다시 쏘지 않으므로 값을 직접 넘겨야 한다.
	void SetPendingCombatStats(int32 InAttack, int32 InWeaponLevel)
	{
		PendingAttack = InAttack;
		PendingWeaponLevel = InWeaponLevel;
		bHasPendingCombatStats = true;
	}

	// 한 번 읽으면 비워짐 — 넘겨받은 값이 없는 최초 시작 시에는 false를 반환해 기본값(기본 공격력, +0)을 유지
	bool ConsumePendingCombatStats(int32& OutAttack, int32& OutWeaponLevel)
	{
		if (!bHasPendingCombatStats)
			return false;

		OutAttack = PendingAttack;
		OutWeaponLevel = PendingWeaponLevel;
		bHasPendingCombatStats = false;
		return true;
	}

private:
	FName PendingPlayerStartTag = NAME_None;

	TArray<FInventorySlot> PendingInventoryItems;
	int32 PendingInventoryGold = 0;
	bool bHasPendingInventory = false;

	int32 PendingLevel = 1;
	int32 PendingExp = 0;
	bool bHasPendingExperience = false;

	int32 PendingMaxHp = 0;
	int32 PendingCurrentHp = 0;
	bool bHasPendingHp = false;

	int32 PendingAttack = 0;
	int32 PendingWeaponLevel = 0;
	bool bHasPendingCombatStats = false;
};
