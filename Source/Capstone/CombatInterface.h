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

};
