#include "UI_QuestWaypoint.h"
#include "Components/TextBlock.h"

void UUI_QuestWaypoint::SetDistanceMeters(int32 Meters)
{
	if (Meters == LastMeters)
		return;

	LastMeters = Meters;

	if (DistanceText)
	{
		DistanceText->SetText(FText::FromString(FString::Printf(TEXT("%dm"), Meters)));
	}
}
