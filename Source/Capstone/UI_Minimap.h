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

	// 위 반지름이 대응하는 실제 월드 거리(cm). 이 거리 이상 떨어진 목표는 미니맵 가장자리에 고정되어 표시된다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float MinimapWorldRadius = 4000.f;

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

private:
	// 위쪽을 향한 흰색 삼각형(알파로 모양을 만듦). 아이콘마다 Tint만 달리해서 공유한다. GC 방지를 위해 UPROPERTY
	UPROPERTY()
	class UTexture2D* TriangleTexture = nullptr;

	// bUseGeneratedTriangleIcons가 켜져 있을 때 MarkerIcon의 Brush를 삼각형 텍스처 + 지정 색으로 교체한다
	void ApplyTriangleBrush(class UImage* Icon, const FLinearColor& Color);

	static class UTexture2D* CreateTriangleTexture(int32 Size);

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
