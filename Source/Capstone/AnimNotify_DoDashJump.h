#pragma once

#include "GameInfo.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_DoDashJump.generated.h"

UCLASS()
class CAPSTONE_API UAnimNotify_DoDashJump : public UAnimNotify
{
	GENERATED_BODY()

public:
	UAnimNotify_DoDashJump();

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference);
	
};
