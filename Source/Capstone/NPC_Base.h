#pragma once

#include "CharBase.h"
#include "NPC_Base.generated.h"

UCLASS()
class CAPSTONE_API ANPC_Base : public ACharBase
{
	GENERATED_BODY()
	
public:
    ANPC_Base();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // 플레이어가 NPC와 상호작용할 때 호출될 가상 함수 (자식들이 override 함)
    UFUNCTION(BlueprintCallable, Category = "NPC")
    virtual void Interact(AActor* Interactor);

protected:
    // NPC의 머리 위에 띄울 이름이나 타입 (에디터에서 수정 가능)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Info")
    FString NPCName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Info")
    FString NPCTypeTitle;
};
