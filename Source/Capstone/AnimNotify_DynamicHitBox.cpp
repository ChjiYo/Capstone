#include "AnimNotify_DynamicHitBox.h"
#include "Monster.h"

UAnimNotify_DynamicHitBox::UAnimNotify_DynamicHitBox()
{}

void UAnimNotify_DynamicHitBox::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
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

void UAnimNotify_DynamicHitBox::NotifyEnd(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation)
{
	Super::NotifyEnd(MeshComp, Animation);
}
