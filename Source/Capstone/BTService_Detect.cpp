#include "BTService_Detect.h"
#include "MonsterAIController.h"
#include "Monster.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"


UBTService_Detect::UBTService_Detect()
{
	NodeName = "Detect";
	Interval = 0.5;
	RandomDeviation = 0.f;
}

void UBTService_Detect::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AMonsterAIController* pController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());

	if (!IsValid(pController))
		return;

	AMonster* pMonster = Cast<AMonster>(pController->GetPawn());

		if (!IsValid(pMonster))
		return;

	FCollisionQueryParams params(NAME_None, false, pMonster);
	TArray<FOverlapResult> resultArray;

	float fTraceRange = pMonster->GetTraceRange();

	bool bOverlap = GetWorld()->OverlapMultiByChannel(resultArray, pMonster->GetActorLocation(), FQuat::Identity, ECollisionChannel::ECC_GameTraceChannel4, FCollisionShape::MakeSphere(fTraceRange), params);

#if ENABLE_DRAW_DEBUG
	FVector vCenter = pMonster->GetActorLocation();
	FColor DrawColor = bOverlap ? FColor::Red : FColor::Green;

	DrawDebugSphere(GetWorld(), vCenter, fTraceRange, 10, DrawColor, false, 0.5f);
#endif
	if (bOverlap)
	{
		pController->SetTarget(resultArray[0].GetActor());
	}
	else
	{
		pController->SetTarget(nullptr);
	}


}


