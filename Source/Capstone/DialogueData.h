#pragma once

#include "Engine/DataTable.h"
#include "DialogueData.generated.h"

USTRUCT(BlueprintType)
struct FDialogueLine : public FTableRowBase
{
    GENERATED_BODY()

    // 비어 있으면 이름표를 숨긴다 (나레이션에서 사용)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    FText SpeakerName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (MultiLine = true))
    FText LineText;

    // true면 초상화/이름표를 숨기고 전체 화면 딤을 켠다 (프롤로그/에필로그)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    bool bIsNarration = false;

    // 0이면 입력 대기. 0보다 크면 그 초 후 자동으로 다음 줄 (전투 중 대사)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    float AutoAdvanceTime = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
    TSoftObjectPtr<UTexture2D> PortraitIcon;
};
