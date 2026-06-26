#pragma once

#include "GameInfo.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_DynamicHitBox.generated.h"

UCLASS()
class CAPSTONE_API UAnimNotifyState_DynamicHitBox : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UAnimNotifyState_DynamicHitBox();


protected:
	UPROPERTY(EditAnywhere, Category = "HitBox")
	FName TargetSocketName = TEXT("Hand_R_Socket");

protected:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference);
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference);
};
