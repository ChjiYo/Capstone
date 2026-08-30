#include "UI_NPC_Nameplate.h"
#include "Components/TextBlock.h"

void UUI_NPC_Nameplate::SetNameplate(const FString& InName, const FString& InRoleTag)
{
	if (NPCNameText)
	{
		NPCNameText->SetText(FText::FromString(InName));
	}

	if (NPCRoleText)
	{
		NPCRoleText->SetText(InRoleTag.IsEmpty() ? FText::GetEmpty() : FText::FromString(FString::Printf(TEXT("<%s>"), *InRoleTag)));
	}
}
