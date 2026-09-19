#include "QuestWaypoint.h"
#include "UI_QuestWaypoint.h"
#include "Py_Grey.h"
#include "QuestComponent.h"
#include "Components/WidgetComponent.h"

AQuestWaypoint::AQuestWaypoint()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	WidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("WaypointWidget"));
	WidgetComp->SetupAttachment(Root);
	WidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetComp->SetDrawAtDesiredSize(true);
	WidgetComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WidgetComp->SetVisibility(false);
}

void AQuestWaypoint::InitWaypoint(APy_Grey* InPlayer, TSubclassOf<UUI_QuestWaypoint> InWidgetClass)
{
	Player = InPlayer;
	WidgetComp->SetWidgetClass(InWidgetClass);
}

void AQuestWaypoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AActor* Target = (Player.IsValid() && Player->QuestComponent) ? Player->QuestComponent->GetQuestTargetActor() : nullptr;

	SetMarkerVisible(Target != nullptr);

	if (!Target)
		return;

	const FVector TargetLoc = Target->GetActorLocation();
	SetActorLocation(TargetLoc + FVector(0.f, 0.f, HeightOffset));

	// 거리는 마커 높이가 아니라 목표 액터 원점 기준 (cm -> m)
	if (UUI_QuestWaypoint* Widget = Cast<UUI_QuestWaypoint>(WidgetComp->GetUserWidgetObject()))
	{
		Widget->SetDistanceMeters(FMath::RoundToInt(FVector::Dist(Player->GetActorLocation(), TargetLoc) / 100.f));
	}
}

void AQuestWaypoint::SetMarkerVisible(bool bVisible)
{
	if (bMarkerVisible == bVisible)
		return;

	bMarkerVisible = bVisible;
	WidgetComp->SetVisibility(bVisible);
}
