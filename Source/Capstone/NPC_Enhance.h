#pragma once

#include "NPC_Base.h"
#include "NPC_Enhance.generated.h"

UCLASS()
class CAPSTONE_API ANPC_Enhance : public ANPC_Base
{
	GENERATED_BODY()

public:
    ANPC_Enhance();

protected:
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUI_Enhance> EnhanceWidgetClass;

    UPROPERTY()
    class UUI_Enhance* EnhanceWidget;

    // 강화 창을 띄운 시점의 플레이어. 닫기 콜백에서 입력모드 복구에 사용
    UPROPERTY()
    AActor* CurrentInteractor;

    // 11단계: 고든과 처음 만났을 때 1회만 재생되는 대사
    UPROPERTY(EditAnywhere, Category = "Story")
    TArray<FName> FirstMeetDialogueLines;

    UPROPERTY(EditAnywhere, Category = "Story")
    FName FirstMeetBeatID = TEXT("Beat_GodonFirstMeet");

    virtual void Interact_Implementation(AActor* Interactor) override;

    void OpenEnhanceWidget();

    UFUNCTION()
    void CloseEnhance();
};
