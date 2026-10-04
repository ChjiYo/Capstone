#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_Minimap.generated.h"

UCLASS()
class CAPSTONE_API UUI_Minimap : public UUserWidget
{
	GENERATED_BODY()

public:
	// APy_Grey::BeginPlay에서 위젯 생성 직후 호출: 소유 플레이어 저장
	void InitMinimap(class APy_Grey* InOwnerPlayer);


protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 화면 정중앙 고정. 플레이어가 바라보는 방향이 항상 지도 위쪽이 되는 방식이라 PlayerIcon 자체는 회전하지 않는다
	// (위쪽을 향한 화살표 텍스처로 그려두면 됨)
	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* PlayerIcon;

	// 퀘스트 목표 방향 마커. MarkerCanvas의 자식이어야 위치를 옮길 수 있다 (Anchor/Alignment 0.5, 0.5 권장)
	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* QuestMarkerIcon;

	// 마을 포탈 등 돌아가는 길 방향 마커. QuestMarkerIcon과 마찬가지로 MarkerCanvas의 자식이어야 한다
	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* PortalMarkerIcon;

	// QuestMarkerIcon/PortalMarkerIcon을 담은 캔버스 패널. 이 아이콘들의 CanvasPanelSlot을 얻기 위해 필요
	UPROPERTY(meta = (BindWidgetOptional))
	class UCanvasPanel* MarkerCanvas;

	// 미니맵 원의 반지름(UMG 단위, px). QuestMarkerIcon이 화면에서 벗어나지 않도록 이 값 안쪽으로 클램프한다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float MinimapRadius = 90.f;

	// PlayerIcon을 항상 이 크기로 강제한다 (Overlay 등 Fill 정렬 컨테이너에 들어있어도 화면을 꽉 채우지 않도록)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FVector2D PlayerIconSize = FVector2D(24.f, 24.f);

	// QuestMarkerIcon을 항상 이 크기로 강제한다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FVector2D QuestMarkerIconSize = FVector2D(20.f, 20.f);

	// PortalMarkerIcon을 항상 이 크기로 강제한다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FVector2D PortalMarkerIconSize = FVector2D(20.f, 20.f);

	// 위 반지름이 대응하는 실제 월드 거리(cm) = 지도 확대 정도. 작을수록 크게 확대된다.
	// 이 거리 이상 떨어진 목표는 미니맵 가장자리에 고정되어 표시된다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap", meta = (ClampMin = "300.0"))
	float MinimapWorldRadius = 2000.f;

	// PlayerIcon/QuestMarkerIcon 화살표 텍스처가 완벽히 위쪽을 향하지 않고 살짝 틀어져 그려졌을 때 보정용 (기본 0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float IconRotationOffsetDegrees = 0.f;

	// true면 PlayerIcon의 Brush 텍스처를 무시하고, 코드가 런타임에 만든 위쪽을 향한 삼각형으로 대체한다.
	// WBP에서 텍스처를 지정할 필요가 없다 (색은 아래 PlayerIconColor가 Brush의 Tint/Color and Opacity를 덮어쓴다).
	// PortalMarkerIcon/QuestMarkerIcon은 대상이 아니며 WBP에 설정한 브러시(모양/색)를 그대로 쓴다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Triangle")
	bool bUseGeneratedTriangleIcons = true;

	// 삼각형 텍스처 한 변의 픽셀 수. 아이콘이 20~24px 정도라 64면 충분히 부드럽다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Triangle", meta = (ClampMin = "16", ClampMax = "256"))
	int32 TriangleTextureSize = 64;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Triangle")
	FLinearColor PlayerIconColor = FLinearColor(1.f, 1.f, 1.f, 1.f);

	// 배경(NativeMapBackground)과 원형 마스크(NativeMapMask)를 코드로 만들어 MarkerCanvas 맨 아래에 끼워 넣을지 여부.
	// WBP에 별도 배경 이미지를 만들 필요 없이 APy_Grey의 위쪽 촬영 카메라 화면을 그대로 써서 "허허벌판" 느낌을 없앤다.
	// MarkerCanvas가 바인딩돼 있어야 동작한다 (기존 QuestMarkerIcon/PortalMarkerIcon과 같은 요구조건)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Map")
	bool bUseGeneratedMapBackground = true;

	// 간략 지도 텍스처 한 변의 픽셀 수 (UMinimapBaker)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Map", meta = (ClampMin = "512", ClampMax = "4096", EditCondition = "bUseGeneratedMapBackground"))
	int32 MapBakeResolution = 2048;

	// 간략 지도를 한 번 굽는 영역 한 변의 월드 길이(cm). 플레이어가 가장자리에 가까워지면 그 자리를 중심으로 다시 굽는다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Map", meta = (ClampMin = "4000.0", EditCondition = "bUseGeneratedMapBackground"))
	float MapBakeWorldSize = 16000.f;

	// 원형 마스크 텍스처(안쪽 투명, 바깥쪽 불투명) 한 변의 픽셀 수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Map", meta = (ClampMin = "64", ClampMax = "512", EditCondition = "bUseGeneratedMapBackground"))
	int32 MapMaskTextureSize = 256;

	// 마스크 바깥쪽(원 밖) 채움색. 배경 캡처 텍스처의 네모난 모서리를 가려 원형 미니맵처럼 보이게 한다
	// (다른 창들의 UI_QuestStyle.h PanelInner와 같은 눈으로 보는 sRGB 값을 리니어로 변환해 둔 것)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Map", meta = (EditCondition = "bUseGeneratedMapBackground"))
	FLinearColor MapMaskOutsideColor = FLinearColor::FromSRGBColor(FColor(0x1C, 0x17, 0x11, 0xFF));

	// 원형 경계에 두르는 얇은 테두리 색 (다른 창의 QuestUI::Gold와 같은 sRGB 값)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Map", meta = (EditCondition = "bUseGeneratedMapBackground"))
	FLinearColor MapMaskRingColor = FLinearColor::FromSRGBColor(FColor(0xCB, 0xA2, 0x4A, 0xFF));

	// 테두리 두께(마스크 텍스처 픽셀 기준)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Map", meta = (ClampMin = "0.0", ClampMax = "16.0", EditCondition = "bUseGeneratedMapBackground"))
	float MapMaskRingThickness = 5.f;

private:
	// 위쪽을 향한 흰색 삼각형(알파로 모양을 만듦). 아이콘마다 Tint만 달리해서 공유한다. GC 방지를 위해 UPROPERTY
	UPROPERTY()
	class UTexture2D* TriangleTexture = nullptr;

	// bUseGeneratedTriangleIcons가 켜져 있을 때 MarkerIcon의 Brush를 삼각형 텍스처 + 지정 색으로 교체한다
	void ApplyTriangleBrush(class UImage* Icon, const FLinearColor& Color);

	static class UTexture2D* CreateTriangleTexture(int32 Size);

	// UMinimapBaker가 구운 간략 지도를 플레이어 주변만 잘라(UV 영역) 보여주는 배경 이미지. WBP가 아니라
	// NativeOnInitialized에서 MarkerCanvas 맨 아래(인덱스 0)에 코드로 끼워 넣는다 (이름은 WBP의 BindWidget 이름과
	// 겹치면 안 되는 프로젝트 관례에 따라 Native 접두사를 붙임 — UI_QuestTracker의 예전 버그 참고)
	UPROPERTY()
	class UImage* NativeMapBackground = nullptr;

	// NativeMapBackground를 담아 미니맵 사각형 밖으로 삐져나오지 않게 자르는 상자.
	// 배경은 플레이어 방향에 맞춰 회전하므로 이게 없으면 돌아간 모서리가 미니맵 밖으로 튀어나온다
	UPROPERTY()
	class USizeBox* NativeMapClip = nullptr;

	// 간략 지도를 굽는 객체. InitMinimap에서 플레이어의 캡처 컴포넌트로 만든다
	UPROPERTY()
	class UMinimapBaker* MapBaker = nullptr;

	// 지금 배경 브러시에 연결된 지도 텍스처 (다시 구워져 바뀌었는지 확인용)
	UPROPERTY()
	class UTexture2D* ShownMapTexture = nullptr;

	// 매 틱 플레이어 위치에 맞춰 배경의 UV 영역/회전을 갱신한다
	void UpdateMapBackground();

	// NativeMapBackground의 네모난 모서리를 가리는 원형 마스크. 배경 바로 위, 마커들 바로 아래에 끼운다
	UPROPERTY()
	class UImage* NativeMapMask = nullptr;

	// 안쪽 투명 / 테두리 골드 / 바깥쪽 불투명인 원형 마스크 텍스처. GC 방지를 위해 UPROPERTY
	UPROPERTY()
	class UTexture2D* CircleMaskTexture = nullptr;

	// bUseGeneratedMapBackground가 켜져 있으면 NativeMapBackground/NativeMapMask를 만들어 MarkerCanvas에 끼운다
	void BuildMapBackground();

	static class UTexture2D* CreateCircleMaskTexture(int32 Size, float RingThicknessPx, const FLinearColor& RingColor, const FLinearColor& OutsideColor);

	TWeakObjectPtr<class APy_Grey> OwnerPlayer;

	// 가장 가까운 "미니맵에 표시" 체크된 포탈. 포탈은 레벨 중간에 새로 생기지 않으므로 한 번만 찾는다
	TWeakObjectPtr<class APortalPoint> CachedPortalTarget;
	bool bPortalSearchAttempted = false;

	// 활성 퀘스트의 목표 위치를 구한다. 없으면 false (마커를 숨겨야 함). 탐색은 UQuestComponent::GetQuestTargetActor가 담당
	bool ResolveQuestTargetLocation(FVector& OutLocation);

	// bShowOnMinimap이 켜진 포탈 중 가장 가까운 것을 찾는다
	class APortalPoint* FindNearestVillagePortal() const;

	// QuestMarkerIcon/PortalMarkerIcon 공용 갱신 로직: 목표가 있으면 방향/거리에 맞춰 위치·회전, 없으면 숨김
	void UpdateDirectionalMarker(class UImage* MarkerIcon, bool bHasTarget, const FVector& TargetLocation);
};
