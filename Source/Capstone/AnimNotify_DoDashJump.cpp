#include "AnimNotify_DoDashJump.h"
#include "Monster.h"


UAnimNotify_DoDashJump::UAnimNotify_DoDashJump()
{

}

void UAnimNotify_DoDashJump::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

    if (MeshComp && MeshComp->GetOwner())
    {
        AMonster* Monster = Cast<AMonster>(MeshComp->GetOwner());
        if (Monster)
        {
            Monster->ExecutePhysicalDash();
        }
    }
}