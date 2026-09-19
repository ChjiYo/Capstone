#pragma once

#include "GameInfo.h"
#include "ItemData.h"
#include "GameFramework/Actor.h"
#include "ItemBase.generated.h"

UCLASS()
class CAPSTONE_API AItemBase : public AActor
{
	GENERATED_BODY()

public:
	AItemBase();

    UFUNCTION(BlueprintCallable, Category = "Item")
    virtual void PickupItem(class ACharacter* Picker);

    // 채집 퀘스트 미니맵 마커가 TargetID와 대조할 수 있도록 노출
    UFUNCTION(BlueprintPure, Category = "Item Data")
    FName GetItemID() const { return ItemID; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
    FName ItemID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
    EItemType ItemType = EItemType::Loot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UNiagaraComponent* ItemVFX;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USphereComponent* CollisionComponent;

    UPROPERTY(EditDefaultsOnly, Category = "Item Data|VFX")
    class UNiagaraSystem* LootDropVFX;

    UPROPERTY(EditDefaultsOnly, Category = "Item Data|VFX")
    class UNiagaraSystem* HealDropVFX;

    UPROPERTY(EditDefaultsOnly, Category = "Item Data|VFX")
    class UNiagaraSystem* WeaponDropVFX;


protected:
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

};
