#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_QuestWaypoint.generated.h"

// 퀘스트 목표 지점 위에 떠서 "몇 m 남았는지"를 보여주는 월드 마커 위젯 (AQuestWaypoint의 WidgetComponent가 사용)
UCLASS()
class CAPSTONE_API UUI_QuestWaypoint : public UUserWidget
{
	GENERATED_BODY()

public:
	// 플레이어와 목표 사이 거리를 m 단위로 표시한다. 직전과 같은 값이면 텍스트를 다시 쓰지 않는다
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void SetDistanceMeters(int32 Meters);

protected:
	// 위젯 블루프린트에 아래 이름과 정확히 같은 이름의 위젯을 만들어야 자동으로 연결됨.
	// BindWidgetOptional이라 위젯이 없어도 블루프린트 컴파일은 깨지지 않고, 해당 항목만 비어 표시됨
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* DistanceText;

private:
	int32 LastMeters = -1;
};
