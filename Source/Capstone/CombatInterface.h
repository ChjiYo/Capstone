#pragma once

#include "GameInfo.h"
#include "UObject/Interface.h"
#include "CombatInterface.generated.h"

UINTERFACE(MinimalAPI)
class UCombatInterface : public UInterface
{
	GENERATED_BODY()
};

class CAPSTONE_API ICombatInterface
{
	GENERATED_BODY()

public:
	virtual void SetExplosiveBuffState(bool bIsActive) = 0;

	// bGrantsShield인 스킬 이펙트가 InitializeEffect 시점에 호출한다. Amount만큼의 보호막을 Duration초 동안 부여하고,
	// 보호막이 깨지거나 시간이 끝나면 ShieldEffect(보호막 VFX 액터)를 대상이 직접 파괴한다.
	// 보호막이 없는 캐릭터도 구현할 필요가 없도록 순수 가상이 아님
	virtual void ActivateShield(int32 Amount, float Duration, class AEffectDestroy* ShieldEffect) {}

};
