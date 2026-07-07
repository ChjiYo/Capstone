#pragma once

#include "NPC_Base.h"
#include "NPC_Quest.generated.h"

UCLASS()
class CAPSTONE_API ANPC_Quest : public ANPC_Base
{
	GENERATED_BODY()

public:
    ANPC_Quest();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FQuestData QuestData;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUI_Quest> QuestWidgetClass;

    UPROPERTY()
    class UUI_Quest* QuestWidget;

    virtual void Interact_Implementation(AActor* Interactor) override;

};
