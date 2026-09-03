#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_Loading.generated.h"

// OpenLevel로 인한 하드 레벨 전환 중 MoviePlayer 로딩 화면 위에 얹히는 위젯.
// LoadingScreenSubsystem이 FLoadingScreenAttributes::bAllowEngineTick을 켠 채로 이 위젯을 넘기므로,
// 게임 스레드가 레벨 로딩으로 블로킹되어 있는 동안에도 NativeTick이 계속 호출된다.
// 여기서 GetAsyncLoadPercentage로 현재 로딩 중인 레벨 패키지의 진행률을 읽어 프로그레스 바를 갱신한다.
UCLASS()
class CAPSTONE_API UUI_Loading : public UUserWidget
{
	GENERATED_BODY()

public:
	// 로딩 진행률을 조회할 레벨 패키지 이름. LoadingScreenSubsystem이 로딩 시작 시 설정한다.
	void SetTargetLevelPackage(const FName& InPackageName) { TargetLevelPackage = InPackageName; }

	// LoadingScreenSubsystem이 새 로딩을 시작할 때마다 호출 — 경과 시간/퍼센트를 초기화한다.
	// (위젯 인스턴스는 레벨 전환마다 재사용되므로 매번 리셋이 필요하다)
	void BeginLoading();

	// LoadingScreenSubsystem이 PostLoadMapWithWorld(로딩 완료)에서 호출 — 100%로 빠르게 마무리시킨다.
	void NotifyLoadFinished() { bLoadFinished = true; }

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	class UProgressBar* LoadingBar;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* PercentText;

	FName TargetLevelPackage;

	// OpenLevel의 메인 레벨 로딩은 LoadPackage(동기)로 처리되어 GetAsyncLoadPercentage가
	// 항상 -1을 반환한다 (비동기 로더에 등록된 적이 없으므로). 그 경우 이 값들로 시간 기반의
	// "체감상 자연스러운" 가짜 진행률을 만들어 대체한다. 실제 비동기 퍼센트를 구할 수 있는
	// 상황(예: 레벨 스트리밍)이 생기면 그 값이 우선 사용된다.
	float ElapsedTime = 0.f;
	bool bLoadFinished = false;

	// 매 프레임 값이 뚝뚝 끊기지 않도록 목표 퍼센트로 보간해 가는 현재 표시값
	float LastPercent = 0.f;
};
