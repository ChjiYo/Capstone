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
    // ĳ���Ͱ� EŰ�� ������ �� ȣ��� �ݱ� �Լ�
    UFUNCTION(BlueprintCallable, Category = "Item")
    void PickupItem(class ACharacter* Picker);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
    FName ItemID;

    // 드롭 연출 나이아가라 선택 기준. DT_Items의 해당 ItemID 타입과 동일하게 지정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
    EItemType ItemType = EItemType::Loot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UNiagaraComponent* ItemVFX;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USphereComponent* CollisionComponent;

    // ItemType별 기본 드롭 이펙트. 필요하면 서브클래스에서 개별 교체 가능
    UPROPERTY(EditDefaultsOnly, Category = "Item Data|VFX")
    class UNiagaraSystem* LootDropVFX;

    UPROPERTY(EditDefaultsOnly, Category = "Item Data|VFX")
    class UNiagaraSystem* HealDropVFX;

    UPROPERTY(EditDefaultsOnly, Category = "Item Data|VFX")
    class UNiagaraSystem* WeaponDropVFX;


protected:
    // �ݸ����� ������ ��
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    // �ݸ������� ������ ��
    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

};
