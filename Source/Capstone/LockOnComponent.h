#pragma once

#include "GameInfo.h"
#include "Components/ActorComponent.h"
#include "LockOnComponent.generated.h"

class AMonster;
class UUI_LockOnMarker;

// 락온 대상이 바뀔 때마다 방송 (nullptr이면 해제)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLockOnTargetChanged, AMonster* /*NewTarget*/)

// 플레이어에 붙는 락온 시스템. 대상을 하나 고정하면 카메라가 그 몬스터를 부드럽게 따라가고 캐릭터도 그쪽을 바라본다.
// - 락온 키: 화면 중앙에 가장 가까운(각도+거리) 몬스터에 건다. 다시 누르면 해제
// - 대상 전환: 마우스 휠 또는 마우스를 옆으로 빠르게 쓸기(플릭) -> 현재 대상 기준 좌/우에서 가장 가까운 몬스터
// - 자동 해제: 대상 사망(다른 대상이 있으면 자동으로 이어서 락온) / 너무 멀어짐 / 벽에 오래 가려짐 / 입력 잠금(UI, 대사, 사망)
// - 공격 어시스트: 락온 대상(없으면 전방 원뿔 안의 가장 가까운 몬스터)을 알려줘서 공격 시 앞으로 파고들게 한다
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class CAPSTONE_API ULockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULockOnComponent();

	// 걸려 있으면 해제하고 false, 아니면 가장 알맞은 몬스터에 건다. 걸 대상이 없어 실패하면 false
	bool ToggleLockOn();

	void ReleaseLock();

	// Direction > 0: 오른쪽 / < 0: 왼쪽. 그쪽에 다른 대상이 없으면 false
	bool SwitchTarget(int32 Direction);

	// 락온 중 마우스 좌우 입력을 흡수한다. true면 호출자는 카메라를 돌리지 않는다.
	// 짧은 시간에 임계값 이상 쓸면 대상 전환으로 해석한다
	bool ConsumeLookInput(float YawInput);

	UFUNCTION(BlueprintPure, Category = "LockOn")
	bool IsLockedOn() const { return LockedTarget.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "LockOn")
	AMonster* GetLockedTarget() const { return LockedTarget.Get(); }

	// 공격 어시스트용 대상. 락온 중이면 그 대상(Range 밖이면 nullptr), 아니면 전방 HalfAngleDeg 원뿔 안에서 가장 가까운 몬스터
	AMonster* FindAssistTarget(float Range, float HalfAngleDeg) const;

	FOnLockOnTargetChanged OnTargetChanged;

public:
	// 이 거리(cm) 안의 몬스터만 락온을 걸 수 있다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Search")
	float MaxLockDistance = 1600.f;

	// 락온 중 이 거리를 넘어 멀어지면 해제. MaxLockDistance보다 커야 경계에서 걸렸다 풀렸다 하지 않는다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Search")
	float BreakDistance = 2100.f;

	// 카메라 정면 기준 이 각도(도) 안의 몬스터만 후보. 단 CloseRangeAnyAngle 안쪽은 방향과 무관하게 후보
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Search")
	float SearchHalfAngle = 75.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Search")
	float CloseRangeAnyAngle = 350.f;

	// 후보 점수 = 각도(도) + 거리(cm) * 이 값. 낮을수록 우선. 화면 중앙에 있는 몬스터를 가장 우선하되 가까운 쪽을 살짝 선호
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Search")
	float DistanceScorePerCm = 0.02f;

	// 대상이 벽 등에 이 시간(초) 이상 가려지면 해제
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Search")
	float LineOfSightLostTimeout = 1.5f;

	// 대상이 죽었을 때 주변의 다른 몬스터에 자동으로 이어서 락온할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Search")
	bool bAutoRetargetOnDeath = true;

	// 자동 이어받기는 전투가 이어지는 근처 몬스터만 대상으로 한다. 멀리 있는 몬스터로 카메라가 갑자기 휙 돌아가지 않게 MaxLockDistance보다 짧게 잡는다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Search")
	float AutoRetargetDistance = 900.f;

	// 카메라가 대상을 따라가는 속도. 클수록 빨리 붙고, 작을수록 부드럽게 끌려온다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Camera")
	float CameraInterpSpeed = 10.f;

	// 락온 카메라의 피치 한계(도). 몹이 발밑/머리 위에 있어도 카메라가 뒤집히지 않게 막는다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Camera")
	float CameraPitchMin = -45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Camera")
	float CameraPitchMax = 20.f;

	// 대상이 화면 중앙에서 위/아래로 얼마나 비켜 보일지(도). 음수면 대상이 화면 위쪽에 놓여 내 캐릭터가 더 잘 보인다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Camera")
	float CameraPitchOffset = -4.f;

	// 이 값(마우스 입력 누적)을 짧은 시간 안에 넘기면 대상 전환. 마우스 감도 설정에 맞춰 조정한다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Switch")
	float FlickThreshold = 4.f;

	// 플릭 입력이 누적되는 시간 창(초). 이 시간에 걸쳐 지수적으로 사라진다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Switch")
	float FlickWindow = 0.12f;

	// 대상 전환 직후 다시 전환되지 않는 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|Switch")
	float SwitchCooldown = 0.35f;

	// 마커 위젯 클래스. 비워두면 C++로 그리는 기본 마커를 쓴다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn|UI")
	TSubclassOf<UUI_LockOnMarker> MarkerWidgetClass;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool IsValidCandidate(const AMonster* Monster) const;
	bool HasLineOfSight(const AMonster* Target) const;
	// 카메라 정면 기준으로 가장 알맞은 후보. 시야가 벽에 가리지 않는 몬스터만 대상이 된다
	AMonster* FindBestTarget(const AMonster* Exclude, float MaxDistance) const;

	void SetLockedTarget(AMonster* NewTarget);
	void UpdateCamera(const AMonster* Target, float RealDelta) const;

	APawn* GetOwnerPawn() const;
	APlayerController* GetOwnerController() const;

	TWeakObjectPtr<AMonster> LockedTarget;

	UPROPERTY()
	UUI_LockOnMarker* MarkerWidget = nullptr;

	float LineOfSightLostTime = 0.f;
	float FlickAccum = 0.f;
	float SwitchCooldownLeft = 0.f;
};
