#include "Mon_NarbashAnim.h"
#include "Mon_Narbash.h"
#include "EffectDestroy.h"

UMon_NarbashAnim::UMon_NarbashAnim()
{
	eAnimType = EBaseAnim::Idle;
	eDir = EDir::Front;

	CurrentCombo = 1;
	NextAttackMax = 3;
	bIsComboInputOn = true;

	static ConstructorHelpers::FObjectFinder <UAnimMontage> SkillAAssetA(TEXT("AnimMontage'/Game/UE_Asset/Character/Player/Grey/MTG_PyGrey_SkillSlotA.MTG_PyGrey_SkillSlotA'"));

	if (SkillAAssetA.Succeeded())
	{
		m_pMTGSkillSlotA = SkillAAssetA.Object;
	}

	static ConstructorHelpers::FObjectFinder <UAnimMontage> SkillAAssetB(TEXT("AnimMontage'/Game/UE_Asset/Character/Player/Grey/MTG_PyGrey_SkillSlotB.MTG_PyGrey_SkillSlotB'"));

	if (SkillAAssetB.Succeeded())
	{
		m_pMTGSkillSlotB = SkillAAssetB.Object;
	}

	static ConstructorHelpers::FObjectFinder <UAnimMontage> SkillAAssetC(TEXT("AnimMontage'/Game/UE_Asset/Character/Player/Grey/MTG_PyGrey_SkillSlotC.MTG_PyGrey_SkillSlotC'"));

	if (SkillAAssetC.Succeeded())
	{
		m_pMTGSkillSlotC = SkillAAssetC.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> NormalAttackAsset(TEXT("AnimMontage'/Game/UE_Asset/Character/Monster/Narbash/MTG_Narbash_Attack.MTG_Narbash_Attack'"));

	if (NormalAttackAsset.Succeeded())
	{
		m_pMTGNormalAttack = NormalAttackAsset.Object;
	}


}

void UMon_NarbashAnim::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UMon_NarbashAnim::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
}

void UMon_NarbashAnim::SkillSlotA()
{
	if (!Montage_IsPlaying(m_pMTGSkillSlotA))
			Montage_Play(m_pMTGSkillSlotA);
}

void UMon_NarbashAnim::SkillSlotB()
{
	if (!Montage_IsPlaying(m_pMTGSkillSlotB))
		Montage_Play(m_pMTGSkillSlotB);
}

void UMon_NarbashAnim::SkillSlotC()
{
	if (!Montage_IsPlaying(m_pMTGSkillSlotC))
		Montage_Play(m_pMTGSkillSlotC);
}

void UMon_NarbashAnim::InherAttackStartNotify()
{
	AMon_Narbash* pOwner = Cast<AMon_Narbash>(TryGetPawnOwner());

	if (IsValid(pOwner))
		pOwner->EnableWeaponCollision(true);
}

void UMon_NarbashAnim::InherNextAttackEndNotify()
{
	bIsComboInputOn = (FMath::RandRange(1, 100) <= 60);
	if (bIsComboInputOn && (CurrentCombo < NextAttackMax))
	{
		CurrentCombo++;
		FName NextSection = *FString::Printf(TEXT("Attack%d"), CurrentCombo);
		Montage_JumpToSection(NextSection, m_pMTGNormalAttack);

		
	}
	else
	{
		bIsComboInputOn = false;
	}
}

void UMon_NarbashAnim::InherAttackEndNotify()
{
	CurrentCombo = 1;
	bIsComboInputOn = true;

	AMonster* pOwner = Cast<AMonster>(TryGetPawnOwner());
	if (IsValid(pOwner))
	{
		pOwner->AttackEnd();
	}

	AMon_Narbash* pNarbashOwner = Cast<AMon_Narbash>(TryGetPawnOwner());
	if (IsValid(pNarbashOwner))
	{
		pNarbashOwner->EnableWeaponCollision(false);
	}
}

void UMon_NarbashAnim::InherSkillAStartNotify()
{
	ACharacter* pPlayer = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

	if (IsValid(pPlayer))
	{
		FVector PlayerLocation = pPlayer->GetActorLocation();

		float CapsuleHalfHeight = pPlayer->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		FVector vImpactPoint = PlayerLocation - FVector(0.f, 0.f, CapsuleHalfHeight);

		FVector vImpactNormal = FVector::UpVector;

		FActorSpawnParameters params;
		params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AEffectDestroy* Effect = GetWorld()->SpawnActor<AEffectDestroy>(
			vImpactPoint,
			FRotator::ZeroRotator,
			params
		);

		Effect->SetEffectPath("ParticleSystem'/Game/FXVarietyPack/Particles/P_ky_fireStorm.P_ky_fireStorm'",
			"SoundWave'/Game/UE_Asset/Sounds/Portal_Sound.Portal_Sound'");
	}
}
