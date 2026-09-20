#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_DamageNumber.generated.h"

// 몬스터가 맞았을 때 화면에 떠오르는 데미지 숫자. 월드 좌표(피격 지점)를 기준점으로 삼아 매 프레임 화면 좌표로
// 투영하고, 그 위에 화면 픽셀 단위의 포물선 오프셋(위로 튀었다가 떨어짐)을 더해서 움직인다.
// 위젯 블루프린트가 없어도 동작한다 - 루트가 비어 있으면 NativeOnInitialized에서 TextBlock을 직접 만든다.
// 폰트/스타일을 바꾸고 싶으면 WBP를 만들어 이 클래스를 부모로 하고 DamageText라는 TextBlock을 두면 된다
UCLASS()
class CAPSTONE_API UUI_DamageNumber : public UUserWidget
{
	GENERATED_BODY()

public:
	// 데미지 숫자 위젯을 만들어 뷰포트에 띄운다. WidgetClass가 비어 있으면 이 네이티브 클래스로 만든다
	static UUI_DamageNumber* Spawn(APlayerController* PlayerController, TSubclassOf<UUI_DamageNumber> WidgetClass, int32 Damage, const FVector& WorldLocation);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 위젯 블루프린트에 정확히 이 이름의 TextBlock이 있으면 자동 연결된다
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* DamageText;

protected:
	// 아래 수치의 길이 단위는 전부 화면 픽셀(슬레이트 단위)이다

	// 화면에 머무는 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageNumber")
	float Lifetime = 0.9f;

	// 처음에 위로 튀어오르는 속도(px/s)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageNumber")
	float LaunchSpeed = 350.f;

	// 좌/우 어느 쪽으로 튈지는 랜덤이고, 그 속도 범위(px/s)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageNumber")
	FVector2D HorizontalSpeedRange = FVector2D(40.f, 100.f);

	// 아래로 떨어지는 가속도(px/s^2). 클수록 포물선이 가파르다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageNumber")
	float Gravity = 1100.f;

	// 생성 직후 이 배율에서 1배로 줄어드는 팝 효과 (1이면 끔)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageNumber")
	float PopScale = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageNumber")
	float PopDuration = 0.12f;

	// 수명의 이 비율이 지난 뒤부터 서서히 투명해진다 (0~1)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageNumber")
	float FadeStartRatio = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageNumber")
	int32 ViewportZOrder = 10;

	// 아래 세 항목은 위젯 블루프린트 없이 네이티브로 만든 TextBlock에만 적용된다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageNumber|Native Style")
	int32 FontSize = 28;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageNumber|Native Style")
	FLinearColor TextColor = FLinearColor(1.f, 0.85f, 0.2f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DamageNumber|Native Style")
	int32 OutlineSize = 2;

private:
	void Setup(int32 Damage, const FVector& WorldLocation);

	// 현재 경과 시간에 맞춰 위치/크기/투명도를 갱신한다
	void UpdateVisual();

	FVector AnchorWorldLocation = FVector::ZeroVector;
	FVector2D Velocity = FVector2D::ZeroVector;
	float Elapsed = 0.f;
};
