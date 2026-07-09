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
