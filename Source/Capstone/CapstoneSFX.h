#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "CapstoneSFX.generated.h"

class USoundBase;
class USoundAttenuation;
class UAudioComponent;
struct FSFXDef;

// 게임 전체에서 쓰는 효과음 ID. 실제 에셋 경로/볼륨/피치/감쇠는 CapstoneSFX.cpp 의 표 하나에만 있다.
// Gen/ 아래 에셋은 Tools/SFX/make_sfx.py 가 합성한 것이고, 나머지는 프로젝트에 원래 있던 에셋(Paragon 보이스, 몬스터 SFX 등)을 재사용한다.
UENUM(BlueprintType)
enum class ESFX : uint8
{
	None,

	// ---- UI ----
	UI_Click, UI_Hover, UI_Open, UI_Close, UI_Deny, UI_Confirm,
	UI_DialogueBlip, UI_DialogueNext, UI_DialogueOpen,
	UI_Coin, UI_Sell, UI_ItemPickup, UI_ItemDrop, UI_Potion, UI_Enhance,
	UI_QuestAccept, UI_QuestUpdate, UI_QuestComplete,
	UI_LevelUp, UI_ChapterTitle, UI_Ending, UI_Death,
	UI_LockOn, UI_LockSwitch, UI_LockRelease, UI_SkillReady,

	// ---- 상호작용 ----
	Interact_Prompt, Interact_Talk, Interact_Gather, Interact_Respawn, Portal_Enter,

	// ---- 플레이어 ----
	Player_Hurt, Player_ShieldHit, Player_ShieldUp, Player_ShieldBreak,
	Player_Land, Player_Jump, Player_SkillA, Player_SkillC, Player_SkillC_Impact, Player_Heartbeat,
	Player_HitFlesh, Player_HitHard,
	Player_VoicePain, Player_VoicePainHeavy, Player_VoiceDeath, Player_VoiceJump, Player_VoiceSkill, Player_VoiceSkillHeavy,

	// ---- 몬스터 ----
	Mon_Alert_Small, Mon_Alert_Big, Mon_Alert_Bird,
	Mon_Death_Small, Mon_Death_Big, Mon_Death_Bird, Mon_Dissolve,
	Mon_Step_Light, Mon_Step_Heavy, Mon_Hit_Generic,
	Mon_Goblin_Attack, Mon_Larvae_Attack, Mon_Larvae_Hit, Mon_Bug_Attack,

	// ---- 마법 / 지속 피해 ----
	Magic_Cast, Magic_Projectile, Magic_Impact, Dot_Tick,

	// ---- 보스 ----
	Boss_Telegraph, Boss_Boom, Boss_Phase, Boss_Death, Boss_DeathVoice, Boss_Throw, Boss_ImpactSmall, Boss_Step,

	// ---- 환경음 / 음악 (레벨 오디오 서브시스템이 사용) ----
	Amb_Wind, Amb_Cave, Amb_Fire, Amb_Dread, Amb_Crow, Music_Boss,

	Count UMETA(Hidden)
};

// 거리 감쇠 등급. 2D는 감쇠 없음
UENUM()
enum class ESFXRange : uint8
{
	Global,
	Short,	// 발소리, 채집 등
	Medium,	// 몬스터 울음/피격
	Large,	// 폭발/충돌
	Boss	// 보스 관련 (멀리서도 들림)
};

// 중앙 재생기. 사운드 에셋은 GameInstance 시작 시 미리 로드해서 첫 재생에서 끊김이 없게 한다.
// 에셋이 없으면(아직 임포트 전 등) 한 번만 경고하고 조용히 무시한다.
UCLASS()
class CAPSTONE_API UCapstoneAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static UCapstoneAudioSubsystem* Get(const UObject* WorldContext);

	// Variant: 여러 에셋이 등록된 SFX에서 특정 번째를 고른다 (-1이면 무작위, 직전과 같은 것은 피함)
	void Play2D(ESFX Id, float Volume = 1.f, float Pitch = 1.f, int32 Variant = -1, bool bPersistAcrossLevel = false);
	void PlayAt(ESFX Id, const FVector& Location, float Volume = 1.f, float Pitch = 1.f, int32 Variant = -1);

	// 루프 사운드/음악. 반환된 컴포넌트를 호출자가 직접 정지시킨다
	UAudioComponent* PlayLoop2D(const UObject* WorldContext, ESFX Id, float Volume = 1.f, float FadeInSeconds = 1.f, bool bRestartWhenFinished = false);

	// 아직 몇 번째 변형이 있는지 (1이면 단일 에셋)
	int32 GetVariantCount(ESFX Id) const;

	// UI 위젯 없이 모든 SButton에 클릭/호버 소리를 붙이는 Slate 입력 프로세서를 켜고 끈다
	void SetUIInputSoundsEnabled(bool bEnabled);

private:
	struct FLoadedSFX
	{
		TArray<TObjectPtr<USoundBase>> Sounds;
		double LastPlayTime = -1000.0;
		int32 LastVariant = -1;
		bool bWarnedMissing = false;
	};

	FLoadedSFX* Resolve(ESFX Id);
	bool ShouldPlay(ESFX Id, FLoadedSFX& Loaded, const FSFXDef& Def) const;
	USoundAttenuation* GetAttenuation(ESFXRange Range) const;

	// 에셋 GC 방지 (FLoadedSFX 는 리플렉션 대상이 아니라서 참조를 따로 잡아둔다)
	UPROPERTY()
	TArray<TObjectPtr<USoundBase>> KeepAlive;

	UPROPERTY()
	TMap<uint8, TObjectPtr<USoundAttenuation>> AttenuationByRange;

	TMap<ESFX, FLoadedSFX> Loaded;
	TSharedPtr<class IInputProcessor> UIInputProcessor;
};

// 호출 지점에서 서브시스템 조회를 반복하지 않도록 한 줄 헬퍼로 감싼다
namespace CapstoneSFX
{
	CAPSTONE_API void Play2D(const UObject* WorldContext, ESFX Id, float Volume = 1.f, float Pitch = 1.f, int32 Variant = -1);
	CAPSTONE_API void PlayAt(const UObject* WorldContext, ESFX Id, const FVector& Location, float Volume = 1.f, float Pitch = 1.f, int32 Variant = -1);
	CAPSTONE_API void PlayPersistent2D(const UObject* WorldContext, ESFX Id, float Volume = 1.f, float Pitch = 1.f);
}

// 레벨(맵 이름)별 환경음/음악. 월드가 시작될 때 맵 이름을 보고 루프를 깔고, 새 지저귐 같은 랜덤 원샷을 뿌린다
UCLASS()
class CAPSTONE_API UCapstoneLevelAudio : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UCapstoneLevelAudio, STATGROUP_Tickables); }

private:
	void StartLoop(ESFX Id, float Volume, bool bRestart = false);

	UPROPERTY()
	TArray<TObjectPtr<UAudioComponent>> Loops;

	bool bBirds = false;
	float BirdVolume = 0.5f;
	float BirdMinInterval = 8.f;
	float BirdMaxInterval = 16.f;
	float BirdTimer = 0.f;
};
