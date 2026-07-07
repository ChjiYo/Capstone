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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
    FString NPCName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
    FString NPCTypeTitle;

    // C++/BP 양쪽에서 재정의 가능한 상호작용 함수
    UFUNCTION(BlueprintNativeEvent, Category = "NPC")
    void Interact(AActor* Interactor);
    virtual void Interact_Implementation(AActor* Interactor);

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;
};
