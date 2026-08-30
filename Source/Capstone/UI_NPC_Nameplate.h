#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_NPC_Nameplate.generated.h"

UCLASS()
class CAPSTONE_API UUI_NPC_Nameplate : public UUserWidget
{
	GENERATED_BODY()

protected:
	// 위젯 블루프린트에 아래 이름과 정확히 같은 이름의 위젯을 만들어야 자동으로 연결됨.
	// BindWidgetOptional이라 위젯이 없어도 블루프린트 컴파일은 깨지지 않고, 해당 항목만 비어 표시됨
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* NPCNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* NPCRoleText;

public:
	// InRoleTag는 "퀘스트"/"상점"/"강화" 같은 순수 역할명만 넘기면 되고, <> 표기는 여기서 붙인다
	UFUNCTION(BlueprintCallable, Category = "NPC")
	void SetNameplate(const FString& InName, const FString& InRoleTag);
};
