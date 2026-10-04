#include "UI_QuestWaypoint.h"
#include "Components/TextBlock.h"

void UUI_QuestWaypoint::SetPortalGuide(bool bInPortalGuide)
{
	if (bPortalGuide == bInPortalGuide)
		return;

	bPortalGuide = bInPortalGuide;
	RefreshText();
}

void UUI_QuestWaypoint::SetDistanceMeters(int32 Meters)
{
	if (Meters == LastMeters)
		return;

	LastMeters = Meters;
	RefreshText();
}

void UUI_QuestWaypoint::RefreshText()
{
	if (DistanceText && LastMeters >= 0)
	{
		DistanceText->SetText(FText::FromString(FString::Printf(TEXT("%s%dm"), bPortalGuide ? TEXT("포탈 ") : TEXT(""), LastMeters)));
	}
}
