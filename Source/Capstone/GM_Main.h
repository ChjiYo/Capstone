#pragma once

#include "GameInfo.h"
#include "GameFramework/GameModeBase.h"
#include "GM_Main.generated.h"

UCLASS()
class CAPSTONE_API AGM_Main : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGM_Main();

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
};
