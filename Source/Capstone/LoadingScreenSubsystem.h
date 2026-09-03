#pragma once

#include "GameInfo.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LoadingScreenSubsystem.generated.h"

class UUI_Loading;

// OpenLevel로 레벨을 이동하면 게임 스레드가 새 레벨을 다 불러올 때까지 블로킹되어 화면이 그대로 멈춰 보인다.
// 이 서브시스템은 PreLoadMap/PostLoadMapWithWorld 시점에 맞춰 MoviePlayer 로딩 화면을 띄우고 내린다.
// MoviePlayer는 게임 스레드가 블로킹된 동안에도 독립적으로 화면을 계속 그려주므로 "끊기는" 느낌이 사라지고,
// bAllowEngineTick을 켜 두면 UUI_Loading::NativeTick도 그 사이에 계속 호출되어 진행률 바를 갱신할 수 있다.
//
// 표시할 위젯 블루프린트는 DefaultGame.ini에서 지정한다:
//   [/Script/Capstone.LoadingScreenSubsystem]
//   LoadingWidgetClass=/Game/UE_Asset/UI/WBP_Loading.WBP_Loading_C
UCLASS(Config = Game)
class CAPSTONE_API ULoadingScreenSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void BeginLoadingScreen(const FString& MapName);
	void EndLoadingScreen(UWorld* LoadedWorld);

	UPROPERTY(Config)
	TSoftClassPtr<UUI_Loading> LoadingWidgetClass;

	UPROPERTY()
	UUI_Loading* LoadingWidgetInstance = nullptr;
};
