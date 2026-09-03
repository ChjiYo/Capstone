#pragma once

#include "CharBaseAnim.h"
#include "MonsterAnim.generated.h"

UCLASS()
class CAPSTONE_API UMonsterAnim : public UCharBaseAnim
{
	GENERATED_BODY()

public:
	UMonsterAnim();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* m_pMTGNormalAttack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimMontage* m_pMTGHit;

	UPROPERTY(EditDefaultsOnly, Category = "Dash|Animation")
	UAnimMontage* DashLeftMontage;

public:
	void Hit();

private:
	UFUNCTION()
	void AnimNotify_HitEnd();

	// Hit 몽타주가 실제로 끝나는(중단되든 끝까지 재생되든) 순간을 정확히 잡기 위한 델리게이트.
	// 노티파이 위치에 의존하지 않으므로, 몽타주에 HitEnd 노티파이가 없거나 잘못 찍혀 있어도 안전하다
	UFUNCTION()
	void OnHitMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// AnimNotify_HitEnd/OnHitMontageEnded/Hit()의 실패 경로가 모두 공유하는 공통 정리 로직
	void ResetAfterHit();
};
