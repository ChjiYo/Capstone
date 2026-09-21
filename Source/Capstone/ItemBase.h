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

    // 몬스터가 죽으면서 떨어뜨리는 전리품을 블루프린트 없이 코드로 만들어 놓는다. 습득하면 InItemID가 InCount개 인벤토리에 들어간다.
    // 바닥 위에 놓이고(Dropper 위치 + ScatterOffset에서 아래로 바닥을 찾는다) 그 아이템의 아이콘이 둥둥 떠 있다. 스폰에 실패하면 nullptr
    static AItemBase* SpawnLoot(AActor* Dropper, FName InItemID, int32 InCount, const FVector& ScatterOffset = FVector::ZeroVector);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
    FName ItemID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
    EItemType ItemType = EItemType::Loot;

    // 주울 때 인벤토리에 들어가는 수량
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
    int32 ItemCount = 1;

    // true면 이 아이템의 아이콘이 픽업 위에 떠 있다. 메시가 따로 있는 블루프린트 픽업은 꺼 두고, 코드로 만든 전리품(SpawnLoot)만 켠다
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
    bool bShowIconSprite = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UBillboardComponent* IconSprite;

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
