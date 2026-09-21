#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_LockOnMarker.generated.h"

// 락온된 몬스터 위에 겹쳐 그려지는 조준 마커. 뷰포트 전체를 덮는 투명 위젯이고, 락온 대상의 월드 좌표를
// 매 프레임 화면 좌표로 투영해 그 자리에 모서리 브래킷(◇ 모양)을 직접 그린다.
// 텍스처/위젯 블루프린트 없이 동작한다 (NativePaint로 슬레이트 선을 그림).
// 락온 순간에는 크게 시작해 대상에 "찰칵" 조여드는 연출과 이후 은은한 맥동이 붙는다
UCLASS()
class CAPSTONE_API UUI_LockOnMarker : public UUserWidget
{
	GENERATED_BODY()

public:
	// 마커 위젯을 만들어 뷰포트 전체를 덮도록 띄운다. WidgetClass가 비어 있으면 이 네이티브 클래스로 만든다
	static UUI_LockOnMarker* Create(APlayerController* PlayerController, TSubclassOf<UUI_LockOnMarker> WidgetClass);

	// nullptr이면 마커를 숨긴다. 대상이 바뀌면 조여드는 연출이 처음부터 다시 재생된다
	void SetLockTarget(class AMonster* NewTarget);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

protected:
	// 길이 단위는 전부 화면 픽셀(슬레이트 단위)

	// 마커 기본 반지름 (대상 MonsterLockOnMarkerScale이 곱해진다)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LockOn")
	float BaseRadius = 34.f;

	// 브래킷 한 변의 길이 비율. 클수록 모서리 선이 길어져 사각형에 가까워진다 (0~0.5)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LockOn")
	float BracketLength = 0.38f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LockOn")
	float LineThickness = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LockOn")
	FLinearColor MarkerColor = FLinearColor(1.f, 0.28f, 0.2f, 1.f);

	// 락온 순간 마커가 조여드는 시작 배율과 걸리는 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LockOn")
	float LockInStartScale = 2.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LockOn")
	float LockInDuration = 0.18f;

	// 조여든 뒤 이어지는 맥동의 크기(배율 편차)와 속도(Hz)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LockOn")
	float PulseAmount = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LockOn")
	float PulseSpeed = 2.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LockOn")
	int32 ViewportZOrder = 5;

private:
	TWeakObjectPtr<class AMonster> Target;

	// NativeTick이 갱신하고 NativePaint(const)가 읽는다
	FVector2D ScreenPosition = FVector2D::ZeroVector;
	float CurrentScale = 1.f;
	float CurrentAlpha = 0.f;
	float LockElapsed = 0.f;
	bool bDrawMarker = false;
};
