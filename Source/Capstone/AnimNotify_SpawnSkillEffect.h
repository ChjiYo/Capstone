#pragma once

#include "GameInfo.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_SpawnSkillEffect.generated.h"

UENUM(BlueprintType)
enum class EEffectTarget : uint8
{
	PlayerUnder UMETA(DisplayName = "플레이어 발 밑"),
	CasterUnder UMETA(DisplayName = "시전자(자신) 발 밑"),
	CasterSocket UMETA(DisplayName = "시전자의 특정 소켓(무기 등)"),
	WeaponSocket UMETA(DisplayName = "시전자의 Weapon 클래스의 소켓")
};

UCLASS()
class CAPSTONE_API UAnimNotify_SpawnSkillEffect : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	UAnimNotify_SpawnSkillEffect();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	TSubclassOf<class AEffectDestroy> EffectToSpawn;

	UPROPERTY(EditAnywhere, Category = "Effect")
	EEffectTarget TargetType;

	UPROPERTY(EditAnywhere, Category = "Effect", meta = (EditCondition = "TargetType == EEffectTarget::CasterSocket || TargetType == EEffectTarget::WeaponSocket"))
	FName SocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	bool bAttachToTarget = false;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
