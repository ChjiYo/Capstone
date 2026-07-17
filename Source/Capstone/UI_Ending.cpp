#include "UI_Ending.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UUI_Ending::NativeConstruct()
{
	Super::NativeConstruct();

	if (RestartButton)
	{
		RestartButton->OnClicked.AddDynamic(this, &UUI_Ending::OnRestartClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UUI_Ending::OnQuitClicked);
	}
}

void UUI_Ending::InitEnding(FName InStartLevelName, int32 FinalLevel, int32 FinalGold)
{
	StartLevelName = InStartLevelName;

	if (SummaryText)
	{
		SummaryText->SetText(FText::Format(
			NSLOCTEXT("UI_Ending", "SummaryFormat", "Lv.{0}   Gold {1}"),
			FinalLevel, FinalGold));
	}
}

void UUI_Ending::OnRestartClicked()
{
	UGameplayStatics::SetGamePaused(GetWorld(), false);
	UGameplayStatics::OpenLevel(GetWorld(), StartLevelName);
}

void UUI_Ending::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}
