#include "AnimNotifyState_DynamicHitBox.h"
#include "Monster.h"

UAnimNotifyState_DynamicHitBox::UAnimNotifyState_DynamicHitBox()
{}

void UAnimNotifyState_DynamicHitBox::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		AMonster* Monster = Cast<AMonster>(MeshComp->GetOwner());
		if (Monster)
		{
			Monster->ActivateHitSphere(TargetSocketName);
		}
	}
}

void UAnimNotifyState_DynamicHitBox::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
}
