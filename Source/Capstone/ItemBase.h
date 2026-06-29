#pragma once

#include "GameInfo.h"
#include "GameFramework/Actor.h"
#include "ItemBase.generated.h"

UCLASS()
class CAPSTONE_API AItemBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AItemBase();
    // 캐릭터가 E키를 눌렀을 때 호출될 줍기 함수
    UFUNCTION(BlueprintCallable, Category = "Item")
    void PickupItem(class ACharacter* Picker);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
    FName ItemID;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* ItemMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USphereComponent* CollisionComponent;

    // 콜리전에 들어왔을 때
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    // 콜리전에서 나갔을 때
    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

};
