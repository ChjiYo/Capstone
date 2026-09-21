#include "UI_LockOnMarker.h"
#include "Monster.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Rendering/DrawElements.h"

UUI_LockOnMarker* UUI_LockOnMarker::Create(APlayerController* PlayerController, TSubclassOf<UUI_LockOnMarker> WidgetClass)
{
	if (!IsValid(PlayerController))
		return nullptr;

	if (!WidgetClass)
		WidgetClass = UUI_LockOnMarker::StaticClass();

	UUI_LockOnMarker* Widget = CreateWidget<UUI_LockOnMarker>(PlayerController, WidgetClass);

	if (!IsValid(Widget))
		return nullptr;

	// 뷰포트 전체를 덮어야 어디에 그려도 잘리지 않는다 (마우스 입력은 NativeConstruct에서 통과시킨다)
	Widget->SetAnchorsInViewport(FAnchors(0.f, 0.f, 1.f, 1.f));
	Widget->SetAlignmentInViewport(FVector2D::ZeroVector);
	Widget->AddToViewport(Widget->ViewportZOrder);

	return Widget;
}

void UUI_LockOnMarker::NativeConstruct()
{
	Super::NativeConstruct();

	// 마커는 매 프레임 위치/크기가 바뀌는 커스텀 페인트라서, 슬레이트의 전역 무효화 캐시가 첫 페인트 결과를 재사용하지 않도록
	// 캐싱을 금지한다 (위치를 바꾸는 위젯 프로퍼티가 없어 자동으로 무효화되지 않는다)
	ForceVolatile(true);

	// 대상이 정해지기 전에는 그릴 것이 없다
	SetVisibility(ESlateVisibility::Collapsed);
}

void UUI_LockOnMarker::SetLockTarget(AMonster* NewTarget)
{
	Target = NewTarget;
	LockElapsed = 0.f;
	bDrawMarker = false;

	// HitTestInvisible: 마커가 마우스 클릭/호버를 가로채지 않는다
	SetVisibility(IsValid(NewTarget) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UUI_LockOnMarker::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	AMonster* Monster = Target.Get();

	if (!IsValid(Monster))
	{
		bDrawMarker = false;
		return;
	}

	// 히트스톱으로 게임 시간이 거의 멈춰도 마커 연출은 실시간으로 흐르도록 위젯 틱의 델타를 그대로 쓴다
	LockElapsed += InDeltaTime;

	APlayerController* PC = GetOwningPlayer();
	FVector2D Projected;

	// 카메라 뒤쪽이면 투영에 실패한다. 이때는 그리지 않는다
	if (!IsValid(PC) || !UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, Monster->GetLockOnPoint(), Projected, false))
	{
		bDrawMarker = false;
		return;
	}

	ScreenPosition = Projected;

	const float LockAlpha = LockInDuration > 0.f ? FMath::Clamp(LockElapsed / LockInDuration, 0.f, 1.f) : 1.f;

	// 처음엔 크게 시작해 빠르게 조여든다 (ease-out)
	const float Eased = 1.f - FMath::Square(1.f - LockAlpha);
	const float Pulse = 1.f + PulseAmount * FMath::Sin(LockElapsed * PulseSpeed * UE_TWO_PI) * LockAlpha;

	CurrentScale = FMath::Lerp(LockInStartScale, 1.f, Eased) * Pulse * Monster->LockOnMarkerScale;
	CurrentAlpha = FMath::Clamp(LockElapsed / 0.08f, 0.f, 1.f);
	bDrawMarker = true;
}

int32 UUI_LockOnMarker::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (!bDrawMarker)
		return LayerId;

	const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();
	const FVector2f Center(ScreenPosition);
	const float Radius = BaseRadius * CurrentScale;

	// 마름모의 네 꼭짓점. 각 꼭짓점에서 양옆 꼭짓점 방향으로 BracketLength만큼만 선을 그어 "ㄴ"자 브래킷 네 개를 만든다
	FVector2f Corners[4];
	for (int32 i = 0; i < 4; ++i)
	{
		const float Angle = UE_HALF_PI * i;
		Corners[i] = Center + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
	}

	auto DrawBrackets = [&](const FLinearColor& Color, float Thickness, int32 Layer)
	{
		for (int32 i = 0; i < 4; ++i)
		{
			const FVector2f& Corner = Corners[i];
			const FVector2f& Prev = Corners[(i + 3) % 4];
			const FVector2f& Next = Corners[(i + 1) % 4];

			TArray<FVector2f> Points;
			Points.Add(Corner + (Prev - Corner) * BracketLength);
			Points.Add(Corner);
			Points.Add(Corner + (Next - Corner) * BracketLength);

			FSlateDrawElement::MakeLines(OutDrawElements, Layer, PaintGeometry, Points, ESlateDrawEffect::None, Color, true, Thickness);
		}
	};

	// 어두운 배경 뒤에서도 보이도록 검은 외곽선을 먼저 깔고 그 위에 색선을 그린다
	FLinearColor Shadow(0.f, 0.f, 0.f, 0.65f * CurrentAlpha);
	FLinearColor Color = MarkerColor;
	Color.A *= CurrentAlpha;

	DrawBrackets(Shadow, LineThickness + 2.5f, LayerId + 1);
	DrawBrackets(Color, LineThickness, LayerId + 2);

	// 중심점: 정확히 어디를 겨냥하는지 보이도록 작은 점을 찍는다
	TArray<FVector2f> Dot;
	Dot.Add(Center + FVector2f(-1.5f, 0.f));
	Dot.Add(Center + FVector2f(1.5f, 0.f));
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, PaintGeometry, Dot, ESlateDrawEffect::None, Color, true, 3.f);

	return LayerId + 3;
}
