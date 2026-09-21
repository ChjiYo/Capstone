#include "CapstoneSFX.h"

#include "AudioDevice.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/ICursor.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "Widgets/SWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogCapstoneSFX, Log, All);

static TAutoConsoleVariable<float> CVarSFXVolume(
	TEXT("Capstone.SFXVolume"), 1.f,
	TEXT("모든 효과음(CapstoneSFX)에 곱해지는 마스터 볼륨 배율. 음악/환경음은 영향받지 않는다."));

static TAutoConsoleVariable<int32> CVarSFXDebug(
	TEXT("Capstone.SFXDebug"), 0,
	TEXT("1이면 재생되는 효과음마다 로그를 남긴다 (LogCapstoneSFX)."));

// ==================================================================================================
// 효과음 표: ESFX -> 에셋 경로 / 볼륨 / 피치 / 감쇠 / 최소 재생 간격
// ==================================================================================================
struct FSFXDef
{
	TArray<FString> Paths;
	float Volume = 1.f;
	float PitchMin = 1.f;
	float PitchMax = 1.f;
	float VolJitter = 0.f;		// 0.1이면 볼륨이 0.9~1.0배로 흔들린다
	ESFXRange Range = ESFXRange::Global;
	float MinInterval = 0.f;	// 같은 SFX를 이 시간(실시간 초) 안에 다시 재생하지 않는다
	bool bLoop = false;
	bool bLazy = false;			// true면 시작 시 미리 올리지 않고 처음 쓸 때 로드 (용량이 큰 음악/환경음 루프)

	FSFXDef& Vol(float V) { Volume = V; return *this; }
	FSFXDef& Pitch(float Lo, float Hi) { PitchMin = Lo; PitchMax = Hi; return *this; }
	FSFXDef& Jitter(float J) { VolJitter = J; return *this; }
	FSFXDef& In(ESFXRange R) { Range = R; return *this; }
	FSFXDef& Gap(float Seconds) { MinInterval = Seconds; return *this; }
	FSFXDef& Loop() { bLoop = true; bLazy = true; return *this; }
};

namespace
{
	FString ObjectPath(const FString& PackagePath)
	{
		return PackagePath + TEXT(".") + FPaths::GetBaseFilename(PackagePath);
	}

	// 이 프로젝트에 원래 있던 에셋 (Paragon 보이스, 몬스터 SFX 등)
	FSFXDef Existing(const TCHAR* PackagePath)
	{
		FSFXDef Def;
		Def.Paths.Add(ObjectPath(PackagePath));
		return Def;
	}

	// Tools/SFX/make_sfx.py 가 만든 에셋. Variants>1 이면 이름_01..이름_NN
	FSFXDef Gen(const TCHAR* Name, int32 Variants = 1)
	{
		FSFXDef Def;
		for (int32 i = 1; i <= Variants; ++i)
		{
			const FString Asset = Variants > 1 ? FString::Printf(TEXT("SFX_%s_%02d"), Name, i) : FString::Printf(TEXT("SFX_%s"), Name);
			Def.Paths.Add(ObjectPath(FString::Printf(TEXT("/Game/UE_Asset/Sounds/Gen/%s"), *Asset)));
		}
		return Def;
	}

	FSFXDef Greystone(const TCHAR* CueName)
	{
		return Existing(*FString::Printf(TEXT("/Game/ParagonGreystone/Audio/Cues/%s"), CueName));
	}

	const TMap<ESFX, FSFXDef>& Table()
	{
		static const TMap<ESFX, FSFXDef> Map = []()
		{
			TMap<ESFX, FSFXDef> M;

			// ---- UI ----
			M.Add(ESFX::UI_Click, Gen(TEXT("UI_Click")).Vol(0.8f).Pitch(0.97f, 1.03f));
			M.Add(ESFX::UI_Hover, Gen(TEXT("UI_Hover")).Vol(0.5f).Pitch(0.98f, 1.04f).Gap(0.04f));
			M.Add(ESFX::UI_Open, Gen(TEXT("UI_Open")).Vol(0.8f).Gap(0.1f));
			M.Add(ESFX::UI_Close, Gen(TEXT("UI_Close")).Vol(0.8f).Gap(0.1f));
			M.Add(ESFX::UI_Deny, Gen(TEXT("UI_Deny")).Vol(0.8f).Gap(0.15f));
			M.Add(ESFX::UI_Confirm, Gen(TEXT("UI_Confirm")).Vol(0.9f));
			M.Add(ESFX::UI_DialogueBlip, Gen(TEXT("UI_DialogueBlip"), 3).Vol(0.7f).Pitch(0.95f, 1.06f).Gap(0.04f));
			M.Add(ESFX::UI_DialogueNext, Gen(TEXT("UI_DialogueNext")).Vol(0.7f).Pitch(0.97f, 1.03f).Gap(0.08f));
			M.Add(ESFX::UI_DialogueOpen, Gen(TEXT("UI_DialogueOpen")).Vol(0.8f).Gap(0.3f));
			M.Add(ESFX::UI_Coin, Gen(TEXT("UI_Coin")).Vol(0.9f).Pitch(0.96f, 1.06f).Gap(0.08f));
			M.Add(ESFX::UI_Sell, Gen(TEXT("UI_Sell")).Vol(0.9f).Gap(0.1f));
			M.Add(ESFX::UI_ItemPickup, Gen(TEXT("UI_ItemPickup")).Vol(0.9f).Pitch(0.97f, 1.05f).Gap(0.05f));
			M.Add(ESFX::UI_ItemDrop, Gen(TEXT("UI_ItemDrop")).Vol(0.9f).Pitch(0.97f, 1.05f).In(ESFXRange::Medium));
			M.Add(ESFX::UI_Potion, Gen(TEXT("UI_Potion")).Vol(0.9f).Gap(0.2f));
			M.Add(ESFX::UI_Enhance, Gen(TEXT("UI_Enhance")).Vol(1.f));
			M.Add(ESFX::UI_QuestAccept, Gen(TEXT("UI_QuestAccept")).Vol(0.9f));
			M.Add(ESFX::UI_QuestUpdate, Gen(TEXT("UI_QuestUpdate")).Vol(0.9f).Gap(0.1f));
			M.Add(ESFX::UI_QuestComplete, Existing(TEXT("/Game/UE_Asset/Sounds/Quest_Complete")).Vol(1.f));
			M.Add(ESFX::UI_LevelUp, Gen(TEXT("UI_LevelUp")).Vol(1.f).Gap(1.f));
			M.Add(ESFX::UI_ChapterTitle, Gen(TEXT("UI_ChapterTitle")).Vol(1.f).Gap(1.f));
			M.Add(ESFX::UI_Ending, Gen(TEXT("UI_Ending")).Vol(1.f));
			M.Add(ESFX::UI_Death, Gen(TEXT("UI_Death")).Vol(1.f));
			M.Add(ESFX::UI_LockOn, Gen(TEXT("UI_LockOn")).Vol(0.7f));
			M.Add(ESFX::UI_LockSwitch, Gen(TEXT("UI_LockSwitch")).Vol(0.6f).Gap(0.05f));
			M.Add(ESFX::UI_LockRelease, Gen(TEXT("UI_LockRelease")).Vol(0.6f).Gap(0.1f));
			M.Add(ESFX::UI_SkillReady, Gen(TEXT("UI_SkillReady")).Vol(0.6f).Gap(0.2f));

			// ---- 상호작용 ----
			M.Add(ESFX::Interact_Prompt, Gen(TEXT("Interact_Prompt")).Vol(0.7f).Gap(0.4f));
			M.Add(ESFX::Interact_Talk, Gen(TEXT("Interact_Talk")).Vol(0.9f).Gap(0.2f));
			M.Add(ESFX::Interact_Gather, Gen(TEXT("Interact_Gather")).Vol(1.f).Pitch(0.95f, 1.08f).In(ESFXRange::Short));
			M.Add(ESFX::Interact_Respawn, Gen(TEXT("Interact_Respawn")).Vol(0.8f).In(ESFXRange::Short));
			M.Add(ESFX::Portal_Enter, Gen(TEXT("Portal_Enter")).Vol(1.f));

			// ---- 플레이어 ----
			M.Add(ESFX::Player_Hurt, Gen(TEXT("Player_Hurt"), 3).Vol(0.9f).Pitch(0.95f, 1.05f).Gap(0.05f));
			M.Add(ESFX::Player_ShieldHit, Gen(TEXT("Player_ShieldHit"), 2).Vol(0.9f).Pitch(0.95f, 1.05f).Gap(0.05f));
			M.Add(ESFX::Player_ShieldUp, Gen(TEXT("Player_ShieldUp")).Vol(0.9f));
			M.Add(ESFX::Player_ShieldBreak, Gen(TEXT("Player_ShieldBreak")).Vol(1.f));
			M.Add(ESFX::Player_Land, Gen(TEXT("Player_Land"), 2).Vol(0.8f).Pitch(0.9f, 1.1f).In(ESFXRange::Short).Gap(0.15f));
			M.Add(ESFX::Player_Jump, Gen(TEXT("Player_Jump")).Vol(0.6f).Pitch(0.95f, 1.08f).Gap(0.15f));
			M.Add(ESFX::Player_SkillA, Gen(TEXT("Player_SkillA")).Vol(0.9f));
			M.Add(ESFX::Player_SkillC, Gen(TEXT("Player_SkillC")).Vol(1.f));
			M.Add(ESFX::Player_SkillC_Impact, Gen(TEXT("Player_SkillC_Impact"), 2).Vol(1.f).Pitch(0.92f, 1.08f).In(ESFXRange::Large).Gap(0.05f));
			M.Add(ESFX::Player_Heartbeat, Gen(TEXT("Player_Heartbeat")).Vol(0.8f).Gap(0.3f));
			// 기존 에셋 (지금까지 어디에도 연결돼 있지 않았던 검 타격음)
			M.Add(ESFX::Player_HitFlesh, Existing(TEXT("/Game/UE_Asset/Sounds/SFX_Player_SwordHit_Flesh")).Vol(0.7f).Pitch(0.92f, 1.08f).In(ESFXRange::Medium).Gap(0.04f));
			M.Add(ESFX::Player_HitHard, Existing(TEXT("/Game/UE_Asset/Sounds/SFX_Player_SwordHit_Hard")).Vol(0.8f).Pitch(0.92f, 1.08f).In(ESFXRange::Medium).Gap(0.04f));
			// Paragon Greystone 보이스 (기존 에셋)
			M.Add(ESFX::Player_VoicePain, Greystone(TEXT("Greystone_Effort_Pain")).Vol(0.9f).Gap(0.6f));
			M.Add(ESFX::Player_VoicePainHeavy, Greystone(TEXT("Greystone_Effort_PainHeavy")).Vol(1.f).Gap(0.6f));
			M.Add(ESFX::Player_VoiceDeath, Greystone(TEXT("Greystone_Effort_Death")).Vol(1.f));
			M.Add(ESFX::Player_VoiceJump, Greystone(TEXT("Greystone_Effort_Jump")).Vol(0.7f).Gap(0.4f));
			M.Add(ESFX::Player_VoiceSkill, Greystone(TEXT("Greystone_Effort_Attack")).Vol(0.9f).Gap(0.3f));
			M.Add(ESFX::Player_VoiceSkillHeavy, Greystone(TEXT("Greystone_Effort_Ability_Q")).Vol(1.f).Gap(0.3f));

			// ---- 몬스터 ----
			M.Add(ESFX::Mon_Alert_Small, Gen(TEXT("Mon_Alert_Small"), 2).Vol(0.9f).Pitch(0.93f, 1.07f).In(ESFXRange::Medium).Gap(0.15f));
			M.Add(ESFX::Mon_Alert_Big, Gen(TEXT("Mon_Alert_Big")).Vol(1.f).Pitch(0.94f, 1.05f).In(ESFXRange::Large).Gap(0.3f));
			M.Add(ESFX::Mon_Alert_Bird, Gen(TEXT("Mon_Alert_Bird")).Vol(0.9f).Pitch(0.93f, 1.08f).In(ESFXRange::Medium).Gap(0.3f));
			M.Add(ESFX::Mon_Death_Small, Gen(TEXT("Mon_Death_Small")).Vol(0.9f).Pitch(0.93f, 1.07f).In(ESFXRange::Medium).Gap(0.08f));
			M.Add(ESFX::Mon_Death_Big, Gen(TEXT("Mon_Death_Big")).Vol(1.f).Pitch(0.94f, 1.05f).In(ESFXRange::Large));
			M.Add(ESFX::Mon_Death_Bird, Gen(TEXT("Mon_Death_Bird")).Vol(0.9f).Pitch(0.94f, 1.06f).In(ESFXRange::Medium).Gap(0.08f));
			M.Add(ESFX::Mon_Dissolve, Gen(TEXT("Mon_Dissolve")).Vol(0.6f).In(ESFXRange::Medium).Gap(0.1f));
			M.Add(ESFX::Mon_Step_Light, Existing(TEXT("/Game/SoulCave/Sound/Cue/Footstep_Dirt_Cue")).Vol(0.55f).Pitch(0.85f, 1.1f).In(ESFXRange::Short).Gap(0.08f));
			M.Add(ESFX::Mon_Step_Heavy, Gen(TEXT("Mon_Step_Heavy"), 3).Vol(0.9f).Pitch(0.92f, 1.08f).In(ESFXRange::Medium).Gap(0.1f));
			M.Add(ESFX::Mon_Hit_Generic, Gen(TEXT("Mon_Hit_Generic"), 2).Vol(0.9f).Pitch(0.93f, 1.07f).In(ESFXRange::Medium).Gap(0.08f));
			// 기존 에셋 (지금까지 연결돼 있지 않았던 몬스터 SFX; 새/골렘/고블린 피격은 애니메이션 노티파이에 이미 연결돼 있다)
			M.Add(ESFX::Mon_Goblin_Attack, Existing(TEXT("/Game/UE_Asset/Sounds/SFX_Mon_Goblin_Attack")).Vol(0.9f).Pitch(0.95f, 1.06f).In(ESFXRange::Medium).Gap(0.2f));
			M.Add(ESFX::Mon_Larvae_Attack, Existing(TEXT("/Game/UE_Asset/Sounds/SFX_Mon_Larvae_Attack")).Vol(0.9f).Pitch(0.95f, 1.06f).In(ESFXRange::Medium).Gap(0.2f));
			M.Add(ESFX::Mon_Larvae_Hit, Existing(TEXT("/Game/UE_Asset/Sounds/SFX_Mon_Larvae_Hit")).Vol(0.9f).Pitch(0.95f, 1.06f).In(ESFXRange::Medium).Gap(0.1f));
			M.Add(ESFX::Mon_Bug_Attack, Existing(TEXT("/Game/UE_Asset/Sounds/SFX_Mon_Bug_Attack")).Vol(0.9f).Pitch(0.95f, 1.06f).In(ESFXRange::Medium).Gap(0.2f));

			// ---- 마법 / 지속 피해 ----
			M.Add(ESFX::Magic_Cast, Gen(TEXT("Magic_Cast")).Vol(0.8f).In(ESFXRange::Medium).Gap(0.2f));
			M.Add(ESFX::Magic_Projectile, Gen(TEXT("Magic_Projectile")).Vol(0.8f).Pitch(0.95f, 1.08f).In(ESFXRange::Medium).Gap(0.1f));
			M.Add(ESFX::Magic_Impact, Gen(TEXT("Magic_Impact")).Vol(1.f).Pitch(0.94f, 1.06f).In(ESFXRange::Large).Gap(0.05f));
			M.Add(ESFX::Dot_Tick, Gen(TEXT("Dot_Tick")).Vol(0.6f).Pitch(0.9f, 1.1f).In(ESFXRange::Short).Gap(0.2f));

			// ---- 보스 ----
			M.Add(ESFX::Boss_Telegraph, Gen(TEXT("Boss_Telegraph")).Vol(1.f).In(ESFXRange::Boss).Gap(0.1f));
			M.Add(ESFX::Boss_Boom, Gen(TEXT("Boss_Boom")).Vol(1.f).Pitch(0.94f, 1.05f).In(ESFXRange::Boss).Gap(0.1f));
			M.Add(ESFX::Boss_Phase, Gen(TEXT("Boss_Phase")).Vol(1.f).In(ESFXRange::Boss));
			M.Add(ESFX::Boss_Death, Gen(TEXT("Boss_Death")).Vol(1.f).In(ESFXRange::Boss));
			M.Add(ESFX::Boss_DeathVoice, Existing(TEXT("/Game/ParagonNarbash/Audio/Cues/Narbash_Death")).Vol(1.2f).In(ESFXRange::Boss));
			M.Add(ESFX::Boss_Throw, Gen(TEXT("Boss_Throw")).Vol(1.f).Pitch(0.95f, 1.05f).In(ESFXRange::Boss).Gap(0.1f));
			M.Add(ESFX::Boss_ImpactSmall, Gen(TEXT("Boss_ImpactSmall")).Vol(1.f).Pitch(0.94f, 1.06f).In(ESFXRange::Boss).Gap(0.05f));
			M.Add(ESFX::Boss_Step, Gen(TEXT("Mon_Step_Heavy"), 3).Vol(1.2f).Pitch(0.7f, 0.82f).In(ESFXRange::Boss).Gap(0.15f));

			// ---- 환경음 / 음악 ----
			M.Add(ESFX::Amb_Wind, Gen(TEXT("Amb_Wind_Loop")).Loop());
			M.Add(ESFX::Amb_Cave, Gen(TEXT("Amb_Cave_Loop")).Loop());
			M.Add(ESFX::Amb_Fire, Gen(TEXT("Amb_Fire_Loop")).Loop());
			M.Add(ESFX::Amb_Dread, Gen(TEXT("Amb_Dread_Loop")).Loop());
			// 밝은 새소리 대신 까마귀 울음 3종 + 낮은 부엉이 울음 1종
			M.Add(ESFX::Amb_Crow, Gen(TEXT("Amb_Crow"), 4).Vol(0.7f).Pitch(0.9f, 1.1f).In(ESFXRange::Large));
			M.Add(ESFX::Music_Boss, Existing(TEXT("/Game/UE_Asset/Sounds/Evan_King_-_End_This_Cue")).Loop());

			return M;
		}();

		return Map;
	}

	// 거리 감쇠 등급 -> (안쪽 반경 = 최대 볼륨 유지, 감쇠 거리) [cm]
	void GetRangeExtents(ESFXRange Range, float& OutInner, float& OutFalloff)
	{
		switch (Range)
		{
		case ESFXRange::Short:	OutInner = 150.f;  OutFalloff = 1500.f; break;
		case ESFXRange::Medium:	OutInner = 300.f;  OutFalloff = 2800.f; break;
		case ESFXRange::Large:	OutInner = 600.f;  OutFalloff = 5000.f; break;
		case ESFXRange::Boss:	OutInner = 1200.f; OutFalloff = 9000.f; break;
		default:				OutInner = 0.f;    OutFalloff = 0.f;    break;
		}
	}
}

// ==================================================================================================
// 모든 SButton(= 모든 UMG UButton)에 클릭/호버 소리를 붙이는 Slate 입력 프로세서
// ==================================================================================================
class FCapstoneUISoundProcessor : public IInputProcessor
{
public:
	explicit FCapstoneUISoundProcessor(UCapstoneAudioSubsystem* InOwner) : Owner(InOwner) {}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}

	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
			return false;

		if (TSharedPtr<SWidget> Button = FindGameButtonUnderCursor(SlateApp, MouseEvent.GetScreenSpacePosition()))
		{
			if (UCapstoneAudioSubsystem* Audio = Owner.Get())
				Audio->Play2D(Button->IsEnabled() ? ESFX::UI_Click : ESFX::UI_Deny);
		}

		return false; // 입력은 소비하지 않는다
	}

	virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		const double Now = FPlatformTime::Seconds();
		if (Now - LastHoverCheck < 0.05)
			return false;
		LastHoverCheck = Now;

		TSharedPtr<SWidget> Button = FindGameButtonUnderCursor(SlateApp, MouseEvent.GetScreenSpacePosition());

		if (Button != HoveredButton.Pin())
		{
			HoveredButton = Button;

			if (Button.IsValid() && Button->IsEnabled())
			{
				if (UCapstoneAudioSubsystem* Audio = Owner.Get())
					Audio->Play2D(ESFX::UI_Hover);
			}
		}

		return false;
	}

	virtual const TCHAR* GetDebugName() const override { return TEXT("CapstoneUISound"); }

private:
	static TSharedPtr<SWidget> FindGameButtonUnderCursor(FSlateApplication& SlateApp, const FVector2D& ScreenPos)
	{
		// 마우스 커서가 숨겨진(게임 조작) 상태에서는 UI를 누를 수 없으므로 검사 자체를 건너뛴다
		const TSharedPtr<ICursor> Cursor = SlateApp.GetPlatformCursor();
		if (Cursor.IsValid() && Cursor->GetType() == EMouseCursor::None)
			return nullptr;

		const FWidgetPath Path = SlateApp.LocateWindowUnderMouse(ScreenPos, SlateApp.GetInteractiveTopLevelWindows());
		if (!Path.IsValid())
			return nullptr;

		// 에디터(PIE) 툴바 같은 에디터 UI 버튼에는 반응하지 않도록, 게임 뷰포트(UMG 레이어) 안에 있는 버튼만 인정한다
		bool bInGameLayer = false;
		TSharedPtr<SWidget> Button;
		FString DebugTypes;

		for (int32 i = Path.Widgets.Num() - 1; i >= 0; --i)
		{
			const TSharedRef<SWidget> Widget = Path.Widgets[i].Widget;
			const FName Type = Widget->GetType();

			if (CVarSFXDebug.GetValueOnGameThread() >= 2)
				DebugTypes += Type.ToString() + TEXT(" < ");

			if (!Button.IsValid() && Type == FName(TEXT("SButton")))
				Button = Widget;

			if (Type == FName(TEXT("SGameLayerManager")))
				bInGameLayer = true;
		}

		if (CVarSFXDebug.GetValueOnGameThread() >= 2)
			UE_LOG(LogCapstoneSFX, Log, TEXT("[SFX] UI 위젯 경로(안쪽부터): %s"), *DebugTypes);

		return bInGameLayer ? Button : nullptr;
	}

	TWeakObjectPtr<UCapstoneAudioSubsystem> Owner;
	TWeakPtr<SWidget> HoveredButton;
	double LastHoverCheck = 0.0;
};

// ==================================================================================================
// UCapstoneAudioSubsystem
// ==================================================================================================
void UCapstoneAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 첫 재생에서 디스크 로드로 끊기지 않도록 표에 있는 에셋을 전부 미리 올린다
	int32 Missing = 0;
	for (const TPair<ESFX, FSFXDef>& Pair : Table())
	{
		// 음악/환경음 루프는 해당 레벨에 들어갈 때 로드한다 (20MB 짜리 음악을 모든 레벨에서 들고 있지 않도록)
		if (Pair.Value.bLazy)
			continue;

		FLoadedSFX* L = Resolve(Pair.Key);
		if (!L || L->Sounds.Num() == 0)
			++Missing;
	}

	UE_LOG(LogCapstoneSFX, Log, TEXT("[SFX] 효과음 사전 로드 완료 (에셋 없음 %d종, 루프/음악은 레벨 진입 시 로드)"), Missing);

	SetUIInputSoundsEnabled(true);
}

void UCapstoneAudioSubsystem::Deinitialize()
{
	SetUIInputSoundsEnabled(false);
	Loaded.Empty();
	KeepAlive.Empty();

	Super::Deinitialize();
}

UCapstoneAudioSubsystem* UCapstoneAudioSubsystem::Get(const UObject* WorldContext)
{
	if (!GEngine)
		return nullptr;

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);
	if (!World)
		return nullptr;

	if (UGameInstance* GI = World->GetGameInstance())
		return GI->GetSubsystem<UCapstoneAudioSubsystem>();

	return nullptr;
}

void UCapstoneAudioSubsystem::SetUIInputSoundsEnabled(bool bEnabled)
{
	if (!FSlateApplication::IsInitialized())
		return;

	if (bEnabled)
	{
		// 데디케이티드 서버/커맨드릿에는 Slate 입력이 없다. 에디터의 PIE 에서는 게임 레이어 안의 버튼만 반응한다
		if (!UIInputProcessor.IsValid() && !IsRunningDedicatedServer() && !IsRunningCommandlet())
		{
			UIInputProcessor = MakeShared<FCapstoneUISoundProcessor>(this);
			FSlateApplication::Get().RegisterInputPreProcessor(UIInputProcessor);
		}
	}
	else if (UIInputProcessor.IsValid())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(UIInputProcessor);
		UIInputProcessor.Reset();
	}
}

UCapstoneAudioSubsystem::FLoadedSFX* UCapstoneAudioSubsystem::Resolve(ESFX Id)
{
	if (FLoadedSFX* Existing = Loaded.Find(Id))
		return Existing;

	const FSFXDef* Def = Table().Find(Id);
	if (!Def)
		return nullptr;

	FLoadedSFX& L = Loaded.Add(Id);

	for (const FString& Path : Def->Paths)
	{
		USoundBase* Sound = LoadObject<USoundBase>(nullptr, *Path);

		if (!Sound)
		{
			if (!L.bWarnedMissing)
			{
				UE_LOG(LogCapstoneSFX, Warning, TEXT("[SFX] 에셋을 찾을 수 없음: %s (Tools/SFX/import_sfx.ps1 로 임포트했는지 확인)"), *Path);
				L.bWarnedMissing = true;
			}
			continue;
		}

		// 임포트 커맨드릿은 루프 플래그를 못 켜므로 런타임에 켠다
		if (Def->bLoop)
		{
			if (USoundWave* Wave = Cast<USoundWave>(Sound))
				Wave->bLooping = true;
		}

		L.Sounds.Add(Sound);
		KeepAlive.AddUnique(Sound);
	}

	return &L;
}

int32 UCapstoneAudioSubsystem::GetVariantCount(ESFX Id) const
{
	const FLoadedSFX* L = Loaded.Find(Id);
	return L ? L->Sounds.Num() : 0;
}

USoundAttenuation* UCapstoneAudioSubsystem::GetAttenuation(ESFXRange Range) const
{
	if (Range == ESFXRange::Global)
		return nullptr;

	if (const TObjectPtr<USoundAttenuation>* Found = AttenuationByRange.Find(static_cast<uint8>(Range)))
		return *Found;

	// const 조회 함수 안에서 lazily 만드는 캐시라 mutable 접근이 필요하다
	UCapstoneAudioSubsystem* MutableThis = const_cast<UCapstoneAudioSubsystem*>(this);

	float Inner = 0.f, Falloff = 0.f;
	GetRangeExtents(Range, Inner, Falloff);

	USoundAttenuation* Attenuation = NewObject<USoundAttenuation>(MutableThis);
	FSoundAttenuationSettings& S = Attenuation->Attenuation;
	S.bAttenuate = true;
	S.bSpatialize = true;
	S.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
	S.AttenuationShape = EAttenuationShape::Sphere;
	S.AttenuationShapeExtents = FVector(Inner, 0.f, 0.f);
	S.FalloffDistance = Falloff;
	// 멀어질수록 고음이 먼저 죽어서 먹먹해지게 한다
	S.bAttenuateWithLPF = true;
	S.LPFRadiusMin = Inner + Falloff * 0.25f;
	S.LPFRadiusMax = Inner + Falloff * 0.9f;
	S.LPFFrequencyAtMin = 20000.f;
	S.LPFFrequencyAtMax = 2500.f;

	MutableThis->AttenuationByRange.Add(static_cast<uint8>(Range), Attenuation);
	return Attenuation;
}

bool UCapstoneAudioSubsystem::ShouldPlay(ESFX Id, FLoadedSFX& L, const FSFXDef& Def) const
{
	if (L.Sounds.Num() == 0)
		return false;

	// 실시간 기준: 히트스톱(전역 시간 감속) 중에도 간격이 늘어나지 않는다
	const double Now = FPlatformTime::Seconds();
	if (Def.MinInterval > 0.f && Now - L.LastPlayTime < Def.MinInterval)
		return false;

	L.LastPlayTime = Now;
	return true;
}

static int32 PickVariant(int32 Requested, int32 Count, int32& LastVariant)
{
	if (Count <= 1)
		return 0;

	if (Requested >= 0)
		return Requested % Count;

	// 무작위지만 직전과 같은 변형은 피해서 연타에서 같은 소리가 반복되지 않게 한다
	int32 Picked = FMath::RandRange(0, Count - 1);
	if (Picked == LastVariant)
		Picked = (Picked + 1 + FMath::RandRange(0, Count - 2)) % Count;

	LastVariant = Picked;
	return Picked;
}

void UCapstoneAudioSubsystem::Play2D(ESFX Id, float Volume, float Pitch, int32 Variant, bool bPersistAcrossLevel)
{
	FLoadedSFX* L = Resolve(Id);
	const FSFXDef* Def = Table().Find(Id);
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;

	if (!L || !Def || !World || !ShouldPlay(Id, *L, *Def))
		return;

	USoundBase* Sound = L->Sounds[PickVariant(Variant, L->Sounds.Num(), L->LastVariant)];
	const float Vol = Volume * Def->Volume * CVarSFXVolume.GetValueOnGameThread() * FMath::FRandRange(1.f - Def->VolJitter, 1.f);
	const float Pit = Pitch * FMath::FRandRange(Def->PitchMin, Def->PitchMax);

	if (CVarSFXDebug.GetValueOnGameThread() > 0)
		UE_LOG(LogCapstoneSFX, Log, TEXT("[SFX] 2D %s (%s) vol=%.2f pitch=%.2f"), *UEnum::GetValueAsString(Id), *Sound->GetName(), Vol, Pit);

	// bPersistAcrossLevel: OpenLevel 직전에 울리는 소리(포탈 등)가 레벨 전환으로 끊기지 않게 한다.
	// PlaySound2D 대신 컴포넌트를 직접 만드는 이유: Paragon 보이스 큐는 영어 자막이 딸려 나오는데,
	// 자막 억제(bSuppressSubtitles)는 Play 이전에 켜야 해서 Spawn* 계열로는 설정할 수 없다
	if (UAudioComponent* Comp = UGameplayStatics::CreateSound2D(World, Sound, Vol, Pit, 0.f, nullptr, bPersistAcrossLevel, true))
	{
		Comp->bSuppressSubtitles = true;
		Comp->Play();
	}
}

void UCapstoneAudioSubsystem::PlayAt(ESFX Id, const FVector& Location, float Volume, float Pitch, int32 Variant)
{
	FLoadedSFX* L = Resolve(Id);
	const FSFXDef* Def = Table().Find(Id);
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;

	if (!L || !Def || !World || !ShouldPlay(Id, *L, *Def))
		return;

	USoundBase* Sound = L->Sounds[PickVariant(Variant, L->Sounds.Num(), L->LastVariant)];
	const float Vol = Volume * Def->Volume * CVarSFXVolume.GetValueOnGameThread() * FMath::FRandRange(1.f - Def->VolJitter, 1.f);
	const float Pit = Pitch * FMath::FRandRange(Def->PitchMin, Def->PitchMax);

	if (CVarSFXDebug.GetValueOnGameThread() > 0)
		UE_LOG(LogCapstoneSFX, Log, TEXT("[SFX] 3D %s (%s) vol=%.2f pitch=%.2f at %s"), *UEnum::GetValueAsString(Id), *Sound->GetName(), Vol, Pit, *Location.ToCompactString());

	UAudioComponent* Comp = FAudioDevice::CreateComponent(Sound, FAudioDevice::FCreateComponentParams(World));
	if (!Comp)
		return;

	Comp->SetWorldLocation(Location);
	Comp->SetVolumeMultiplier(Vol);
	Comp->SetPitchMultiplier(Pit);
	Comp->bAutoDestroy = true;
	Comp->bSuppressSubtitles = true;

	if (USoundAttenuation* Attenuation = GetAttenuation(Def->Range))
	{
		Comp->bAllowSpatialization = true;
		Comp->AttenuationSettings = Attenuation;
	}
	else
	{
		Comp->bAllowSpatialization = false;
	}

	Comp->Play();
}

UAudioComponent* UCapstoneAudioSubsystem::PlayLoop2D(const UObject* WorldContext, ESFX Id, float Volume, float FadeInSeconds, bool bRestartWhenFinished)
{
	FLoadedSFX* L = Resolve(Id);
	const FSFXDef* Def = Table().Find(Id);
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;

	if (!L || !Def || !World || L->Sounds.Num() == 0)
		return nullptr;

	// 환경음/음악은 CVarSFXVolume(효과음 마스터)의 영향을 받지 않는다
	UAudioComponent* Comp = UGameplayStatics::CreateSound2D(World, L->Sounds[0], Volume * Def->Volume, 1.f, 0.f, nullptr, false, false);
	if (!Comp)
		return nullptr;

	Comp->bSuppressSubtitles = true;

	if (bRestartWhenFinished)
	{
		// 루프 플래그가 없는 음악 큐가 끝까지 재생되면 처음부터 다시 튼다
		TWeakObjectPtr<UAudioComponent> WeakComp = Comp;
		Comp->OnAudioFinishedNative.AddLambda([WeakComp](UAudioComponent*)
		{
			if (WeakComp.IsValid())
				WeakComp->Play();
		});
	}

	if (FadeInSeconds > 0.f)
		Comp->FadeIn(FadeInSeconds, 1.f);
	else
		Comp->Play();

	// 루프는 레벨당 몇 번뿐이라 디버그 CVar 와 무관하게 항상 남긴다
	UE_LOG(LogCapstoneSFX, Log, TEXT("[SFX] LOOP %s (%s) vol=%.2f"), *UEnum::GetValueAsString(Id), *L->Sounds[0]->GetName(), Volume);

	return Comp;
}

// ---------------------------------------------------------------------- 편의 함수
void CapstoneSFX::Play2D(const UObject* WorldContext, ESFX Id, float Volume, float Pitch, int32 Variant)
{
	if (UCapstoneAudioSubsystem* Audio = UCapstoneAudioSubsystem::Get(WorldContext))
		Audio->Play2D(Id, Volume, Pitch, Variant);
}

void CapstoneSFX::PlayAt(const UObject* WorldContext, ESFX Id, const FVector& Location, float Volume, float Pitch, int32 Variant)
{
	if (UCapstoneAudioSubsystem* Audio = UCapstoneAudioSubsystem::Get(WorldContext))
		Audio->PlayAt(Id, Location, Volume, Pitch, Variant);
}

void CapstoneSFX::PlayPersistent2D(const UObject* WorldContext, ESFX Id, float Volume, float Pitch)
{
	if (UCapstoneAudioSubsystem* Audio = UCapstoneAudioSubsystem::Get(WorldContext))
		Audio->Play2D(Id, Volume, Pitch, -1, true);
}

// ==================================================================================================
// UCapstoneLevelAudio : 레벨별 환경음/음악
// ==================================================================================================
namespace
{
	struct FLevelAudioProfile
	{
		const TCHAR* MapName;
		ESFX Loop1 = ESFX::None; float Loop1Volume = 0.f; bool bLoop1Restart = false;
		ESFX Loop2 = ESFX::None; float Loop2Volume = 0.f;
		ESFX Loop3 = ESFX::None; float Loop3Volume = 0.f;
		float BirdVolume = 0.f;
		float BirdMin = 10.f;
		float BirdMax = 20.f;
	};

	// 마을/들판/보스룸만 새로 깐다. LV_Cave 는 레벨에 이미 환경음(물방울/동굴 바람)과 음악이 배치돼 있고,
	// 마을/들판의 음악(Keys_Of_Moon / Castles_In_The_Sky)도 레벨 안의 AmbientSound 가 재생하므로 건드리지 않는다.
	const FLevelAudioProfile* FindProfile(const FString& MapName)
	{
		static const FLevelAudioProfile Profiles[] =
		{
			// 불탄 마을: 바람 + 멀리서 타는 불 + 불길한 저역 패드 + 드문 까마귀 울음
			{ TEXT("Lv_Village"),   ESFX::Amb_Wind, 0.45f, false, ESFX::Amb_Fire, 0.5f, ESFX::Amb_Dread, 0.7f, 0.6f, 12.f, 24.f },
			// 들판: 바람 + 저역 패드 + 까마귀 울음
			{ TEXT("Lv_OpenField"), ESFX::Amb_Wind, 0.5f,  false, ESFX::Amb_Dread, 0.6f, ESFX::None, 0.f, 0.6f, 8.f, 18.f },
			{ TEXT("Lv_Field"),     ESFX::Amb_Wind, 0.5f,  false, ESFX::Amb_Dread, 0.6f, ESFX::None, 0.f, 0.6f, 8.f, 18.f },
			{ TEXT("Lv_DefaultField"), ESFX::Amb_Wind, 0.5f, false, ESFX::Amb_Dread, 0.6f, ESFX::None, 0.f, 0.6f, 8.f, 18.f },
			// 보스룸: 레벨에 음악/환경음이 하나도 없어서 동굴 드론 + 보스 테마를 깐다
			{ TEXT("Lv_Boss"),      ESFX::Amb_Cave, 0.6f,  false, ESFX::Music_Boss, 0.55f, ESFX::None, 0.f, 0.f, 10.f, 20.f },
		};

		for (const FLevelAudioProfile& P : Profiles)
		{
			if (MapName.Equals(P.MapName, ESearchCase::IgnoreCase))
				return &P;
		}

		return nullptr;
	}
}

bool UCapstoneLevelAudio::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && !IsRunningDedicatedServer() && Super::ShouldCreateSubsystem(Outer);
}

void UCapstoneLevelAudio::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	const FString MapName = UGameplayStatics::GetCurrentLevelName(&InWorld, true);
	const FLevelAudioProfile* Profile = FindProfile(MapName);

	UE_LOG(LogCapstoneSFX, Log, TEXT("[SFX] 레벨 오디오: %s -> %s"), *MapName, Profile ? TEXT("프로필 있음") : TEXT("프로필 없음 (레벨에 배치된 소리만 사용)"));

	if (!Profile)
		return;

	if (Profile->Loop1 != ESFX::None)
		StartLoop(Profile->Loop1, Profile->Loop1Volume, Profile->bLoop1Restart);

	if (Profile->Loop2 != ESFX::None)
		StartLoop(Profile->Loop2, Profile->Loop2Volume, Profile->Loop2 == ESFX::Music_Boss);

	if (Profile->Loop3 != ESFX::None)
		StartLoop(Profile->Loop3, Profile->Loop3Volume);

	if (Profile->BirdVolume > 0.f)
	{
		bBirds = true;
		BirdVolume = Profile->BirdVolume;
		BirdMinInterval = Profile->BirdMin;
		BirdMaxInterval = Profile->BirdMax;
		BirdTimer = FMath::FRandRange(2.f, 6.f);
	}
}

void UCapstoneLevelAudio::Deinitialize()
{
	for (UAudioComponent* Comp : Loops)
	{
		if (IsValid(Comp))
			Comp->Stop();
	}

	Loops.Empty();

	Super::Deinitialize();
}

void UCapstoneLevelAudio::StartLoop(ESFX Id, float Volume, bool bRestart)
{
	UCapstoneAudioSubsystem* Audio = UCapstoneAudioSubsystem::Get(GetWorld());
	if (!Audio)
		return;

	if (UAudioComponent* Comp = Audio->PlayLoop2D(GetWorld(), Id, Volume, 2.5f, bRestart))
		Loops.Add(Comp);
}

void UCapstoneLevelAudio::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bBirds)
		return;

	BirdTimer -= DeltaTime;
	if (BirdTimer > 0.f)
		return;

	BirdTimer = FMath::FRandRange(BirdMinInterval, BirdMaxInterval);

	UWorld* World = GetWorld();
	const APawn* Pawn = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
	UCapstoneAudioSubsystem* Audio = UCapstoneAudioSubsystem::Get(World);

	if (!Pawn || !Audio)
		return;

	// 플레이어 주변 15~35m, 위쪽 어딘가 (나무 위에서 우는 느낌)
	const float Angle = FMath::FRandRange(0.f, 2.f * PI);
	const float Dist = FMath::FRandRange(1500.f, 3500.f);
	const FVector Location = Pawn->GetActorLocation() + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, FMath::FRandRange(400.f, 1200.f));

	Audio->PlayAt(ESFX::Amb_Crow, Location, BirdVolume);
}

// ==================================================================================================
// 콘솔 명령 (~ 키): 소리를 직접 들어보고 조정하는 용도
// ==================================================================================================
static void PlaySFXByName(UWorld* World, const FString& Name)
{
	const UEnum* Enum = StaticEnum<ESFX>();
	const int64 Value = Enum ? Enum->GetValueByNameString(Name) : INDEX_NONE;

	if (Value == INDEX_NONE || Value == static_cast<int64>(ESFX::None))
	{
		UE_LOG(LogCapstoneSFX, Warning, TEXT("[SFX] 알 수 없는 이름: %s (Capstone.ListSFX 로 목록 확인)"), *Name);
		return;
	}

	if (UCapstoneAudioSubsystem* Audio = UCapstoneAudioSubsystem::Get(World))
	{
		const ESFX Id = static_cast<ESFX>(Value);
		const APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);

		// 거리 감쇠가 있는 소리는 플레이어 전방 5m 앞에서, 나머지는 2D로 재생한다
		const FSFXDef* Def = Table().Find(Id);
		if (Def && Def->Range != ESFXRange::Global && Pawn)
			Audio->PlayAt(Id, Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 500.f);
		else
			Audio->Play2D(Id);
	}
}

static FAutoConsoleCommandWithWorldAndArgs CmdPlaySFX(
	TEXT("Capstone.PlaySFX"),
	TEXT("Capstone.PlaySFX <ESFX 이름>  예) Capstone.PlaySFX UI_LevelUp"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() > 0)
			PlaySFXByName(World, Args[0]);
	}));

static FAutoConsoleCommandWithWorld CmdListSFX(
	TEXT("Capstone.ListSFX"),
	TEXT("사용 가능한 ESFX 이름 전체를 로그로 출력한다"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		const UEnum* Enum = StaticEnum<ESFX>();
		for (int32 i = 1; Enum && i < Enum->NumEnums() - 1; ++i)
			UE_LOG(LogCapstoneSFX, Log, TEXT("[SFX] %s"), *Enum->GetNameStringByIndex(i));
	}));

static FAutoConsoleCommandWithWorldAndArgs CmdPlayAllSFX(
	TEXT("Capstone.PlayAllSFX"),
	TEXT("Capstone.PlayAllSFX [간격(초, 기본 1.5)] : 모든 효과음을 차례로 재생한다 (밸런스 점검/감상용)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World)
			return;

		const float Interval = Args.Num() > 0 ? FMath::Max(0.1f, FCString::Atof(*Args[0])) : 1.5f;

		TSharedRef<int32> Index = MakeShared<int32>(1);
		TSharedRef<FTimerHandle> Handle = MakeShared<FTimerHandle>();
		TWeakObjectPtr<UWorld> WeakWorld = World;

		World->GetTimerManager().SetTimer(*Handle, FTimerDelegate::CreateLambda([Index, Handle, WeakWorld]()
		{
			const UEnum* Enum = StaticEnum<ESFX>();
			UWorld* W = WeakWorld.Get();

			if (!W || !Enum || *Index >= Enum->NumEnums() - 1)
			{
				if (W)
					W->GetTimerManager().ClearTimer(*Handle);
				UE_LOG(LogCapstoneSFX, Log, TEXT("[SFX] PlayAll 종료"));
				return;
			}

			const FString Name = Enum->GetNameStringByIndex(*Index);
			UE_LOG(LogCapstoneSFX, Log, TEXT("[SFX] PlayAll -> %s"), *Name);
			PlaySFXByName(W, Name);
			++(*Index);
		}), Interval, true, 1.0f);
	}));
