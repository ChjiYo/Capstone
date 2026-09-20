#include "BossCameraShake.h"

UBossShakePattern::UBossShakePattern(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UBossShakePattern::GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const
{
	OutInfo.Duration = FCameraShakeDuration(Duration);
	OutInfo.BlendIn = BlendInTime;
	OutInfo.BlendOut = BlendOutTime;
}

void UBossShakePattern::StartShakePatternImpl(const FCameraShakePatternStartParams& Params)
{
	Elapsed = 0.f;
}

void UBossShakePattern::UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult)
{
	Elapsed += Params.DeltaTime;

	// 시간에 따라 선형으로 약해지는 사인파. 축마다 위상을 달리해서 한 방향으로만 흔들리는 부자연스러움을 피한다
	const float Decay = FMath::Clamp(1.f - Elapsed / FMath::Max(Duration, KINDA_SMALL_NUMBER), 0.f, 1.f);
	const float Phase = Elapsed * Frequency * UE_TWO_PI;

	OutResult.Rotation.Pitch = FMath::Sin(Phase) * PitchAmplitude * Decay;
	OutResult.Rotation.Yaw = FMath::Sin(Phase * 0.83f + 1.3f) * YawAmplitude * Decay;
	OutResult.Rotation.Roll = FMath::Sin(Phase * 1.17f + 2.1f) * RollAmplitude * Decay;

	OutResult.Location.Z = FMath::Sin(Phase * 1.1f + 0.7f) * LocationAmplitude * Decay;
	OutResult.Location.Y = FMath::Sin(Phase * 0.9f + 2.4f) * LocationAmplitude * 0.5f * Decay;
}

bool UBossShakePattern::IsFinishedImpl() const
{
	return Elapsed >= Duration;
}

UBossShakeBase::UBossShakeBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSingleInstance = false;
	SetRootShakePattern(CreateDefaultSubobject<UBossShakePattern>(TEXT("BossPattern")));
}

UBossShake_Light::UBossShake_Light(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (UBossShakePattern* Pattern = GetBossPattern())
	{
		Pattern->Duration = 0.30f;
		Pattern->PitchAmplitude = 0.9f;
		Pattern->YawAmplitude = 0.5f;
		Pattern->RollAmplitude = 0.35f;
		Pattern->LocationAmplitude = 2.f;
		Pattern->Frequency = 24.f;
	}
}

UBossShake_Heavy::UBossShake_Heavy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (UBossShakePattern* Pattern = GetBossPattern())
	{
		Pattern->Duration = 0.65f;
		Pattern->BlendOutTime = 0.4f;
		Pattern->PitchAmplitude = 2.6f;
		Pattern->YawAmplitude = 1.4f;
		Pattern->RollAmplitude = 1.1f;
		Pattern->LocationAmplitude = 7.f;
		Pattern->Frequency = 17.f;
	}
}

UBossShake_Roar::UBossShake_Roar(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (UBossShakePattern* Pattern = GetBossPattern())
	{
		Pattern->Duration = 1.6f;
		Pattern->BlendInTime = 0.15f;
		Pattern->BlendOutTime = 0.8f;
		Pattern->PitchAmplitude = 1.5f;
		Pattern->YawAmplitude = 1.f;
		Pattern->RollAmplitude = 0.8f;
		Pattern->LocationAmplitude = 5.f;
		Pattern->Frequency = 13.f;
	}
}
