#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_Death.generated.h"

// 플레이어가 죽었을 때 화면 전체를 어둡게 덮으며 "OOO에 의해 사망하셨습니다." 문구와 [계속] 버튼을 띄우는 위젯.
// 버튼을 누르면 OnContinue가 방송되고, APy_Grey가 이를 받아 마을로 리스폰시킨다.
// 위젯 블루프린트가 없어도 동작한다 - 루트가 비어 있으면 NativeOnInitialized에서 배경(Border)/TextBlock/Button을 직접 만든다.
// 폰트/배경을 바꾸고 싶으면 WBP를 만들어 이 클래스를 부모로 하고 DeathText(TextBlock), ContinueButton(Button)을 두면 된다
UCLASS()
class CAPSTONE_API UUI_Death : public UUserWidget
{
	GENERATED_BODY()

public:
	// 문구를 채운다. KillerName이 비어 있으면(몬스터가 아닌 원인으로 사망) UnknownKillerMessage를 쓴다
	void SetKillerName(const FText& KillerName);

	// [계속] 버튼을 눌렀을 때 방송된다
	DECLARE_MULTICAST_DELEGATE(FOnDeathContinue);
	FOnDeathContinue OnContinue;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 위젯 블루프린트에 정확히 이 이름의 TextBlock이 있으면 자동 연결된다
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* DeathText;

	// 위젯 블루프린트에 정확히 이 이름의 Button이 있으면 자동 연결된다. 페이드인이 끝나기 전에는 비활성 상태다
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* ContinueButton;

protected:
	// {0}이 죽인 몬스터 이름으로 치환된다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Death")
	FText KillerMessageFormat = INVTEXT("{0}에 의해 사망하셨습니다.");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Death")
	FText UnknownKillerMessage = INVTEXT("사망하셨습니다.");

	// 네이티브로 만든 [계속] 버튼의 글자
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Death")
	FText ContinueButtonText = INVTEXT("계속");

	// 화면이 서서히 어두워지며 문구가 나타나는 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Death")
	float FadeInDuration = 0.8f;

	// 사망 후 [계속] 버튼이 눌리기 시작하는 시간(초). 게임 중 마우스 커서는 화면 중앙에 있고 버튼도 그 근처에 뜨므로,
	// 죽는 순간 공격(좌클릭)을 연타하던 입력이 버튼을 바로 눌러 문구를 못 보고 넘어가는 것을 막는다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Death")
	float ButtonEnableDelay = 1.5f;

	// 아래 세 항목은 위젯 블루프린트 없이 네이티브로 만든 위젯에만 적용된다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Death|Native Style")
	int32 FontSize = 44;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Death|Native Style")
	FLinearColor TextColor = FLinearColor(0.85f, 0.1f, 0.1f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Death|Native Style")
	FLinearColor BackdropColor = FLinearColor(0.f, 0.f, 0.f, 0.65f);

private:
	void BuildNativeTree();

	UFUNCTION()
	void HandleContinueClicked();

	float Elapsed = 0.f;
	bool bButtonEnabled = false;
};
