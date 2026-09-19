#pragma once

#include "GameInfo.h"
#include "GameFramework/Actor.h"
#include "QuestWaypoint.generated.h"

// 활성 퀘스트가 진행돼야 하는 지점(UQuestComponent::GetQuestTargetActor) 위에 거리 마커를 띄우는 액터.
// 스크린 스페이스 WidgetComponent라 NPC 이름표처럼 월드 좌표를 따라다니며 항상 같은 크기로 그려진다.
// APy_Grey::BeginPlay가 스폰하며, 퀘스트가 없거나 완료되면 스스로 숨는다.
UCLASS()
class CAPSTONE_API AQuestWaypoint : public AActor
{
	GENERATED_BODY()

public:
	AQuestWaypoint();

	// SpawnActorDeferred 직후, FinishSpawning 전에 호출해야 위젯이 올바른 클래스로 만들어진다
	void InitWaypoint(class APy_Grey* InPlayer, TSubclassOf<class UUI_QuestWaypoint> InWidgetClass);

	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UWidgetComponent* WidgetComp;

	// 목표 액터의 원점(cm)에서 마커까지 띄우는 높이. 몬스터 HP 바/NPC 이름표와 겹치지 않도록 넉넉히 잡았다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint")
	float HeightOffset = 250.f;

private:
	void SetMarkerVisible(bool bVisible);

	TWeakObjectPtr<class APy_Grey> Player;
	bool bMarkerVisible = false;
};
