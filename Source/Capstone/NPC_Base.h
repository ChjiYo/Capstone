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

    UPROPERTY(Category = Collision, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    class USphereComponent* InteractionSphere;

  
    UPROPERTY(Category = "NPC", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    class UWidgetComponent* NameplateComponent;

public:
    UFUNCTION(BlueprintNativeEvent, Category = "NPC")
    void Interact(AActor* Interactor);

    virtual void Interact_Implementation(AActor* Interactor);

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

protected:
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
