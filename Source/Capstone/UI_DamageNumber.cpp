#include "UI_DamageNumber.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Styling/CoreStyle.h"

UUI_DamageNumber* UUI_DamageNumber::Spawn(APlayerController* PlayerController, TSubclassOf<UUI_DamageNumber> WidgetClass, int32 Damage, const FVector& WorldLocation)
{
	if (!IsValid(PlayerController))
		return nullptr;

	if (!WidgetClass)
		WidgetClass = UUI_DamageNumber::StaticClass();

	UUI_DamageNumber* Widget = CreateWidget<UUI_DamageNumber>(PlayerController, WidgetClass);

	if (!IsValid(Widget))
		return nullptr;

	Widget->Setup(Damage, WorldLocation);
	Widget->AddToViewport(Widget->ViewportZOrder);

	// 텍스트 중앙을 기준점에 맞추고, 첫 프레임에 (0,0)에 깜빡이지 않도록 바로 한 번 위치를 잡아 둔다
	Widget->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
	Widget->UpdateVisual();

	return Widget;
}

void UUI_DamageNumber::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 위젯 블루프린트로 이미 트리를 구성한 경우에는 건드리지 않는다
	if (!WidgetTree || WidgetTree->RootWidget)
		return;

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DamageText"));

	FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", FontSize);
	Font.OutlineSettings.OutlineSize = OutlineSize;
	Font.OutlineSettings.OutlineColor = FLinearColor::Black;

	Text->SetFont(Font);
	Text->SetColorAndOpacity(FSlateColor(TextColor));
	Text->SetJustification(ETextJustify::Center);

	WidgetTree->RootWidget = Text;
	DamageText = Text;
}

void UUI_DamageNumber::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	Elapsed += InDeltaTime;

	if (Elapsed >= Lifetime)
	{
		RemoveFromParent();
		return;
	}

	UpdateVisual();
}

void UUI_DamageNumber::Setup(int32 Damage, const FVector& WorldLocation)
{
	AnchorWorldLocation = WorldLocation;

	if (DamageText)
		DamageText->SetText(FText::AsNumber(Damage));

	// 화면 좌표는 y가 아래로 커지므로, 위로 튀는 속도는 음수다
	const float Side = FMath::RandBool() ? 1.f : -1.f;
	Velocity = FVector2D(Side * FMath::RandRange(HorizontalSpeedRange.X, HorizontalSpeedRange.Y), -LaunchSpeed);

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UUI_DamageNumber::UpdateVisual()
{
	APlayerController* PC = GetOwningPlayer();

	FVector2D ScreenPos;
	if (!IsValid(PC) || !UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, AnchorWorldLocation, ScreenPos, false))
	{
		// 기준점이 카메라 뒤에 있으면 보이지 않게만 하고, 시간은 계속 흐르게 둔다
		SetRenderOpacity(0.f);
		return;
	}

	// 등가속도 운동: 위치 = v0*t + 0.5*g*t^2 (y는 아래 방향이 +)
	const FVector2D Offset(Velocity.X * Elapsed, Velocity.Y * Elapsed + 0.5f * Gravity * Elapsed * Elapsed);

	// ProjectWorldLocationToWidgetPosition은 이미 DPI 스케일이 적용된 위젯 좌표를 돌려주므로 다시 나누지 않는다
	SetPositionInViewport(ScreenPos + Offset, false);

	const float PopAlpha = PopDuration > 0.f ? FMath::Clamp(Elapsed / PopDuration, 0.f, 1.f) : 1.f;
	const float Scale = FMath::Lerp(PopScale, 1.f, PopAlpha);
	SetRenderScale(FVector2D(Scale, Scale));

	const float FadeStart = Lifetime * FMath::Clamp(FadeStartRatio, 0.f, 1.f);
	const float FadeLength = FMath::Max(Lifetime - FadeStart, UE_KINDA_SMALL_NUMBER);
	SetRenderOpacity(1.f - FMath::Clamp((Elapsed - FadeStart) / FadeLength, 0.f, 1.f));
}
