#include "LoadingScreenSubsystem.h"
#include "UI_Loading.h"
#include "MoviePlayer.h"
#include "Misc/PackageName.h"
#include "Misc/App.h"

void ULoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &ULoadingScreenSubsystem::BeginLoadingScreen);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ULoadingScreenSubsystem::EndLoadingScreen);
}

void ULoadingScreenSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	Super::Deinitialize();
}

void ULoadingScreenSubsystem::BeginLoadingScreen(const FString& MapName)
{
	if (IsRunningDedicatedServer())
		return;

	if (LoadingWidgetClass.IsNull())
	{
		LOG(TEXT("LoadingWidgetClass가 DefaultGame.ini에 설정되지 않았습니다. 로딩 화면 없이 진행합니다."));
		return;
	}

	if (!LoadingWidgetInstance)
	{
		TSubclassOf<UUI_Loading> WidgetClass = LoadingWidgetClass.LoadSynchronous();
		if (!WidgetClass)
			return;

		LoadingWidgetInstance = CreateWidget<UUI_Loading>(GetGameInstance(), WidgetClass);
	}

	if (!LoadingWidgetInstance)
		return;

	// PreLoadMap이 넘겨주는 MapName은 짧은 이름일 수 있으므로, GetAsyncLoadPercentage가 실제로
	// 조회하는 롱 패키지 이름으로 변환을 시도한다 (실패하면 받은 이름 그대로 사용)
	FString LongPackageName;
	if (!FPackageName::SearchForPackageOnDisk(MapName, &LongPackageName))
	{
		LongPackageName = MapName;
	}

	LoadingWidgetInstance->SetTargetLevelPackage(FName(*LongPackageName));
	LoadingWidgetInstance->BeginLoading();

	FLoadingScreenAttributes LoadingScreen;
	LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
	LoadingScreen.bAllowEngineTick = true;
	LoadingScreen.MinimumLoadingScreenDisplayTime = 0.5f;
	LoadingScreen.WidgetLoadingScreen = LoadingWidgetInstance->TakeWidget();

	// SetupLoadingScreen은 속성을 저장할 뿐 재생을 시작하지 않으므로 PlayMovie를 직접 호출해야 한다.
	GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
	GetMoviePlayer()->PlayMovie();
}

void ULoadingScreenSubsystem::EndLoadingScreen(UWorld* LoadedWorld)
{
	// bAutoCompleteWhenLoadingCompletes = true 이므로 MoviePlayer가 로딩 완료 시 알아서 내려간다.
	// 여기서는 가짜 진행률을 100%로 빠르게 마무리시키는 역할만 한다.
	if (LoadingWidgetInstance)
	{
		LoadingWidgetInstance->NotifyLoadFinished();
	}
}
