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
    TArray<FQuestData> QuestChain;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
    int32 CurrentChainIndex = 0;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUI_Quest> QuestWidgetClass;

    UPROPERTY()
    class UUI_Quest* QuestWidget;

    UPROPERTY()
    AActor* CurrentInteractor;

    virtual void Interact_Implementation(AActor* Interactor) override;

public:
    // 현재 체인 퀘스트를 수락/거절 UI 없이 Interactor에게 즉시 지급한다 (AStoryTrigger가 대사 종료 콜백으로 호출)
    void GrantCurrentQuestFor(AActor* Interactor);

protected:
    UFUNCTION()
    void HandleQuestAccepted();

    UFUNCTION()
    void HandleQuestDeclined();

    void RestoreInputMode();

    // 11단계: 인트로 대사(있으면) 재생 후 수락창을 연다. 대사가 없으면 곧바로 OpenQuestWidget으로
    void BeginNextQuestIntro();

    // 기존 수락창 생성 블록을 그대로 옮긴 것. CurrentInteractor/CurrentChainIndex를 그대로 사용
    void OpenQuestWidget();

};
