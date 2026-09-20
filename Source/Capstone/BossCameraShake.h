#pragma once

#include "GameInfo.h"
#include "Camera/CameraShakeBase.h"
#include "BossCameraShake.generated.h"

// 보스 연출 전용 카메라 셰이크 패턴. 에셋(BP) 없이 코드만으로 만들 수 있도록 감쇠하는 사인파 진동을 직접 구현한다.
// 진폭/주파수/지속시간은 서브클래스 생성자에서 채운다
UCLASS()
class CAPSTONE_API UBossShakePattern : public UCameraShakePattern
{
	GENERATED_BODY()

public:
	UBossShakePattern(const FObjectInitializer& ObjectInitializer);

	float Duration = 0.4f;
	float BlendInTime = 0.02f;
	float BlendOutTime = 0.25f;

	// 회전(도) / 위치(cm) 진폭과 초당 진동 횟수
	float PitchAmplitude = 1.f;
	float YawAmplitude = 0.6f;
	float RollAmplitude = 0.4f;
	float LocationAmplitude = 3.f;
	float Frequency = 22.f;

private:
	float Elapsed = 0.f;

	virtual void GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const override;
	virtual void StartShakePatternImpl(const FCameraShakePatternStartParams& Params) override;
	virtual void UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult) override;
	virtual bool IsFinishedImpl() const override;
};

UCLASS(Abstract)
class CAPSTONE_API UBossShakeBase : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	UBossShakeBase(const FObjectInitializer& ObjectInitializer);

protected:
	// 서브클래스 생성자에서 호출해 패턴 파라미터를 채운다
	UBossShakePattern* GetBossPattern() const { return Cast<UBossShakePattern>(GetRootShakePattern()); }
};

// 가벼운 타격 (콤보 명중)
UCLASS()
class CAPSTONE_API UBossShake_Light : public UBossShakeBase
{
	GENERATED_BODY()

public:
	UBossShake_Light(const FObjectInitializer& ObjectInitializer);
};

// 묵직한 타격 (스매시/돌진 명중, 도약 착지)
UCLASS()
class CAPSTONE_API UBossShake_Heavy : public UBossShakeBase
{
	GENERATED_BODY()

public:
	UBossShake_Heavy(const FObjectInitializer& ObjectInitializer);
};

// 포효/광폭화/궁극기: 길고 큰 진동
UCLASS()
class CAPSTONE_API UBossShake_Roar : public UBossShakeBase
{
	GENERATED_BODY()

public:
	UBossShake_Roar(const FObjectInitializer& ObjectInitializer);
};
