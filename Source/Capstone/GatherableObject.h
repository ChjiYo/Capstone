#pragma once

#include "ItemBase.h"
#include "GatherableObject.generated.h"

// 채집이 일어날 때마다 전역 방송. 5단계 채집 퀘스트가 이 델리게이트를 구독해 진행도를 갱신한다
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnItemGathered, FName /*ItemID*/, int32 /*Count*/)

UCLASS()
class CAPSTONE_API AGatherableObject : public AItemBase
{
	GENERATED_BODY()

public:
	AGatherableObject();

	virtual void PickupItem(class ACharacter* Picker) override;

	static FOnItemGathered OnItemGathered;

	// 몬스터 드롭 아이템과 달리 채집물은 플레이어가 맵에서 직접 찾아야 하므로
	// 이펙트(ItemVFX)만으로 대체하지 않고 실제 메시(약초/덤불 등)를 함께 노출한다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* GatherMesh;

	// 1회 채집당 지급 수량
	UPROPERTY(EditAnywhere, Category = "Gather")
	int32 GatherCount = 1;

	// 채집 후 재생성까지 시간(초). 0 이하면 리스폰 없이 파괴(1회용)
	UPROPERTY(EditAnywhere, Category = "Gather")
	float RespawnTime = 30.f;

private:
	void Respawn();

	FTimerHandle RespawnTimerHandle;
};
