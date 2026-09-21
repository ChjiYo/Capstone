#include "LockOnComponent.h"
#include "Monster.h"
#include "UI_LockOnMarker.h"
#include "EngineUtils.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Misc/App.h"

ULockOnComponent::ULockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// 락온 중에만 틱을 켠다 (SetLockedTarget)
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

APawn* ULockOnComponent::GetOwnerPawn() const
{
	return Cast<APawn>(GetOwner());
}

APlayerController* ULockOnComponent::GetOwnerController() const
{
	const APawn* Pawn = GetOwnerPawn();

	return Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
}

bool ULockOnComponent::IsValidCandidate(const AMonster* Monster) const
{
	return IsValid(Monster) && Monster->bCanBeLockedOn && Monster->GetCurrentHp() > 0 && !Monster->IsHidden();
}

bool ULockOnComponent::HasLineOfSight(const AMonster* Target) const
{
	const APawn* Pawn = GetOwnerPawn();
	UWorld* World = GetWorld();

	if (!Pawn || !World || !IsValid(Target))
		return false;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(LockOnLineOfSight), false, Pawn);

	TArray<AActor*> Attached;

	// 다른 몬스터(와 그들이 든 무기 액터)가 앞을 막아도 시야가 가려진 것으로 치지 않는다. 벽/지형만 가림으로 본다
	for (TActorIterator<AMonster> It(World); It; ++It)
	{
		Params.AddIgnoredActor(*It);

		Attached.Reset();
		It->GetAttachedActors(Attached);
		Params.AddIgnoredActors(Attached);
	}

	Attached.Reset();
	Pawn->GetAttachedActors(Attached);
	Params.AddIgnoredActors(Attached);

	const FVector Eye = Pawn->GetActorLocation() + FVector(0.f, 0.f, 50.f);

	FHitResult Hit;

	return !World->LineTraceSingleByChannel(Hit, Eye, Target->GetLockOnPoint(), ECC_Visibility, Params);
}

AMonster* ULockOnComponent::FindBestTarget(const AMonster* Exclude, float MaxDistance) const
{
	const APawn* Pawn = GetOwnerPawn();
	const APlayerController* PC = GetOwnerController();
	UWorld* World = GetWorld();

	if (!Pawn || !PC || !World)
		return nullptr;

	const FVector Origin = Pawn->GetActorLocation();
	const FVector ViewDirection = PC->GetControlRotation().Vector();

	AMonster* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();

	for (TActorIterator<AMonster> It(World); It; ++It)
	{
		AMonster* Monster = *It;

		if (Monster == Exclude || !IsValidCandidate(Monster))
			continue;

		const FVector ToTarget = Monster->GetLockOnPoint() - Origin;
		const float Distance = ToTarget.Size();

		if (Distance > MaxDistance)
			continue;

		const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(ViewDirection, ToTarget.GetSafeNormal()), -1.f, 1.f)));

		if (Angle > SearchHalfAngle && Distance > CloseRangeAnyAngle)
			continue;

		const float Score = Angle + Distance * DistanceScorePerCm;

		// LOS 트레이스는 비싼 편이라 점수가 더 좋은 후보에만 수행한다
		if (Score >= BestScore || !HasLineOfSight(Monster))
			continue;

		Best = Monster;
		BestScore = Score;
	}

	return Best;
}

bool ULockOnComponent::ToggleLockOn()
{
	if (IsLockedOn())
	{
		SetLockedTarget(nullptr);
		return false;
	}

	AMonster* Best = FindBestTarget(nullptr, MaxLockDistance);

	if (!Best)
		return false;

	SetLockedTarget(Best);
	return true;
}

void ULockOnComponent::ReleaseLock()
{
	SetLockedTarget(nullptr);
}

bool ULockOnComponent::SwitchTarget(int32 Direction)
{
	AMonster* Current = LockedTarget.Get();
	const APawn* Pawn = GetOwnerPawn();
	UWorld* World = GetWorld();

	if (!IsValid(Current) || !Pawn || !World || Direction == 0)
		return false;

	const FVector Origin = Pawn->GetActorLocation();
	const float CurrentYaw = (Current->GetLockOnPoint() - Origin).GetSafeNormal2D().Rotation().Yaw;

	AMonster* Best = nullptr;
	float BestDelta = TNumericLimits<float>::Max();

	for (TActorIterator<AMonster> It(World); It; ++It)
	{
		AMonster* Monster = *It;

		if (Monster == Current || !IsValidCandidate(Monster))
			continue;

		const FVector ToTarget = Monster->GetLockOnPoint() - Origin;

		if (ToTarget.Size() > MaxLockDistance)
			continue;

		// 현재 대상 방향을 기준으로 한 부호 있는 각도. 양수 = 오른쪽(요 증가 방향)
		const float Delta = FMath::FindDeltaAngleDegrees(CurrentYaw, ToTarget.GetSafeNormal2D().Rotation().Yaw);

		// 반대편이거나 사실상 같은 방향에 겹친 몬스터는 제외
		if (Delta * Direction <= 0.5f)
			continue;

		const float AbsDelta = FMath::Abs(Delta);

		if (AbsDelta >= BestDelta || !HasLineOfSight(Monster))
			continue;

		Best = Monster;
		BestDelta = AbsDelta;
	}

	if (!Best)
		return false;

	SetLockedTarget(Best);
	SwitchCooldownLeft = SwitchCooldown;

	return true;
}

bool ULockOnComponent::ConsumeLookInput(float YawInput)
{
	if (!IsLockedOn())
		return false;

	FlickAccum += YawInput;

	if (SwitchCooldownLeft <= 0.f && FMath::Abs(FlickAccum) >= FlickThreshold)
	{
		SwitchTarget(FlickAccum > 0.f ? 1 : -1);
		FlickAccum = 0.f;
	}

	return true;
}

AMonster* ULockOnComponent::FindAssistTarget(float Range, float HalfAngleDeg) const
{
	const APawn* Pawn = GetOwnerPawn();
	UWorld* World = GetWorld();

	if (!Pawn || !World)
		return nullptr;

	const FVector Origin = Pawn->GetActorLocation();

	if (AMonster* Locked = LockedTarget.Get())
		return IsValidCandidate(Locked) && FVector::Dist2D(Origin, Locked->GetActorLocation()) <= Range ? Locked : nullptr;

	const FVector Forward = Pawn->GetActorForwardVector().GetSafeNormal2D();
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(HalfAngleDeg));

	AMonster* Best = nullptr;
	float BestDistance = Range;

	for (TActorIterator<AMonster> It(World); It; ++It)
	{
		AMonster* Monster = *It;

		if (!IsValidCandidate(Monster))
			continue;

		const FVector ToTarget = Monster->GetActorLocation() - Origin;
		const float Distance = ToTarget.Size2D();

		if (Distance > BestDistance || Distance < 1.f)
			continue;

		if (FVector::DotProduct(Forward, ToTarget.GetSafeNormal2D()) < MinDot)
			continue;

		Best = Monster;
		BestDistance = Distance;
	}

	return Best;
}

void ULockOnComponent::SetLockedTarget(AMonster* NewTarget)
{
	AMonster* Old = LockedTarget.Get();

	if (Old == NewTarget)
		return;

	if (IsValid(Old))
		Old->SetLockedOn(false);

	LockedTarget = NewTarget;
	LineOfSightLostTime = 0.f;
	FlickAccum = 0.f;

	if (IsValid(NewTarget))
		NewTarget->SetLockedOn(true);

	SetComponentTickEnabled(IsValid(NewTarget));

	if (IsValid(NewTarget) && !IsValid(MarkerWidget))
		MarkerWidget = UUI_LockOnMarker::Create(GetOwnerController(), MarkerWidgetClass);

	if (IsValid(MarkerWidget))
		MarkerWidget->SetLockTarget(NewTarget);

	OnTargetChanged.Broadcast(NewTarget);
}

void ULockOnComponent::UpdateCamera(const AMonster* Target, float RealDelta) const
{
	const APawn* Pawn = GetOwnerPawn();
	APlayerController* PC = GetOwnerController();

	if (!Pawn || !PC || !PC->PlayerCameraManager)
		return;

	const FVector Point = Target->GetLockOnPoint();

	// 캐릭터가 대상을 정확히 바라보도록 요(Yaw)는 캐릭터 위치 기준, 화면에 대상이 자리잡도록 피치는 카메라 위치 기준으로 구한다
	FVector Flat = Point - Pawn->GetActorLocation();
	Flat.Z = 0.f;

	const FRotator Current = PC->GetControlRotation();
	FRotator Desired = Current;

	// 대상과 거의 겹쳐 있으면 방향이 마구 튀므로 요는 그대로 둔다
	if (Flat.SizeSquared() > 25.f)
		Desired.Yaw = Flat.Rotation().Yaw;

	const float TargetPitch = (Point - PC->PlayerCameraManager->GetCameraLocation()).Rotation().Pitch;
	Desired.Pitch = FMath::Clamp(TargetPitch + CameraPitchOffset, CameraPitchMin, CameraPitchMax);
	Desired.Roll = 0.f;

	PC->SetControlRotation(FMath::RInterpTo(Current, Desired, RealDelta, CameraInterpSpeed));
}

void ULockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AMonster* Target = LockedTarget.Get();
	const APawn* Pawn = GetOwnerPawn();

	if (!Pawn)
		return;

	// 히트스톱으로 게임 시간이 거의 멈춰도 카메라가 굳지 않도록 실제 프레임 시간으로 움직인다
	const float RealDelta = FApp::GetDeltaTime();

	// 대상이 죽었거나 사라졌다: 주변에 다른 몬스터가 있으면 이어서 락온, 없으면 해제
	if (!IsValidCandidate(Target))
	{
		SetLockedTarget(bAutoRetargetOnDeath ? FindBestTarget(Target, AutoRetargetDistance) : nullptr);
		return;
	}

	if (FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation()) > BreakDistance)
	{
		SetLockedTarget(nullptr);
		return;
	}

	if (HasLineOfSight(Target))
	{
		LineOfSightLostTime = 0.f;
	}
	else
	{
		LineOfSightLostTime += RealDelta;

		if (LineOfSightLostTime >= LineOfSightLostTimeout)
		{
			SetLockedTarget(nullptr);
			return;
		}
	}

	SwitchCooldownLeft = FMath::Max(0.f, SwitchCooldownLeft - RealDelta);
	FlickAccum *= FMath::Exp(-RealDelta / FMath::Max(FlickWindow, 0.01f));

	UpdateCamera(Target, RealDelta);
}

void ULockOnComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetLockedTarget(nullptr);

	if (IsValid(MarkerWidget))
		MarkerWidget->RemoveFromParent();

	MarkerWidget = nullptr;

	Super::EndPlay(EndPlayReason);
}
