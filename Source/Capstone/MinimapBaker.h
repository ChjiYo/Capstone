#pragma once

#include "GameInfo.h"
#include "UObject/Object.h"
#include "MinimapBaker.generated.h"

// 미니맵 배경 "간략 지도"를 굽는 객체.
// 플레이어 주변 넓은 영역을 위에서 정사영으로 한 번 찍고(조명 없는 색 + 높이), 그것을 백그라운드 스레드에서
// 단순한 색 면 + 외곽선 + 지형 음영으로 바꾼 텍스처를 만든다. 매 프레임 찍던 예전 방식과 달리 레벨 조명(밤/동굴)과
// 무관하게 항상 같은 모습이고, 플레이어가 구운 영역 가장자리에 가까워질 때만 다시 굽는다.
// 실내(머리 위가 막힌 곳, 예: 동굴)는 플레이어 머리 조금 위에서 잘라 찍어 천장이 지도를 가리지 않게 한다.
UCLASS()
class CAPSTONE_API UMinimapBaker : public UObject
{
	GENERATED_BODY()

public:
	struct FSettings
	{
		// 구운 텍스처 한 변의 픽셀 수
		int32 Resolution = 2048;

		// 한 번 굽는 영역 한 변의 월드 길이(cm)
		float BakeWorldSize = 16000.f;

		// 구운 영역 중심에서 이만큼(cm) 벗어나면 새 위치로 다시 굽는다
		float RebakeDistance = 4000.f;

		// 실내(머리 위가 막힌 곳)에서는 플레이어 위 이 높이(cm)에서 잘라 찍는다 (천장 제거)
		float IndoorCutHeight = 450.f;

		// 실내에서 플레이어 높이가 구울 때보다 이만큼(cm) 달라지면 다시 굽는다 (층/경사로)
		float IndoorRebakeHeight = 350.f;

		// 야외에서는 플레이어 위 이 높이(cm)에서 찍는다 (건물/나무보다 확실히 높게)
		float OutdoorCameraHeight = 6000.f;
	};

	void Init(class USceneCaptureComponent2D* InCapture, AActor* InPlayer, const FSettings& InSettings);

	// 매 틱 호출: 다시 구울지 판단하고, 진행 중인 굽기(GPU 읽기 → 변환 → 텍스처)를 한 단계씩 진행한다
	void Tick(const FVector& PlayerLocation);

	// 다 구워진 지도 텍스처. 첫 굽기가 끝나기 전에는 nullptr
	class UTexture2D* GetMapTexture() const { return MapTexture; }

	// 월드 XY → 텍스처 UV(0~1). 텍스처 위쪽이 월드 +X, 오른쪽이 월드 +Y
	FVector2D WorldToUV(const FVector& WorldLocation) const;

	// 텍스처 한 변이 덮는 월드 길이(cm)
	float GetMapWorldSize() const { return Settings.BakeWorldSize; }

	virtual void BeginDestroy() override;

	// 렌더 스레드/백그라운드 변환과 주고받는 굽기 한 번의 데이터 (MinimapBaker.cpp에서 정의)
	struct FBakeJob;

private:

	void StartBake(const FVector& PlayerLocation, bool bIndoor);
	void PollReadback();

	// 구울 영역 안의 내비메시 다각형을 모은다 (걸을 수 있는 바닥 = 지도의 길/바닥). 내비메시가 없는 레벨이면 비워 둔다
	void GatherNavPolys(FBakeJob& Job) const;
	void FinishBake();

	// 야외로 찍은 결과에서 플레이어 머리 위가 대부분 막혀 있으면(동굴 천장 등) 실내로 본다.
	// 충돌 없는 천장 메시도 있어서 라인 트레이스 대신 찍힌 높이로 판단한다
	static bool LooksIndoor(const FBakeJob& Job);

	FSettings Settings;

	TWeakObjectPtr<class USceneCaptureComponent2D> Capture;
	TWeakObjectPtr<AActor> Player;

	UPROPERTY()
	class UTextureRenderTarget2D* CaptureTarget = nullptr;

	UPROPERTY()
	class UTexture2D* MapTexture = nullptr;

	// 지금 화면에 보이는 텍스처의 중심(월드 XY)과, 그때 플레이어 높이/실내 여부
	FVector2D MapCenter = FVector2D::ZeroVector;
	float MapPlayerZ = 0.f;
	bool bMapIndoor = false;
	bool bHasMap = false;

	// 진행 중인 굽기 (렌더 스레드/백그라운드 스레드와 공유)
	TSharedPtr<FBakeJob, ESPMode::ThreadSafe> PendingJob;

	// 실내 판정 실패 등으로 같은 자리에서 굽기를 무한 반복하지 않도록 마지막 시도 시각
	double LastBakeStartTime = -1000.0;
};
