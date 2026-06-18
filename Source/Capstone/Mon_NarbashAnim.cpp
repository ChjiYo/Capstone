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

/*void UMon_NarbashAnim::InherSkillAStartNotify()
{
	// 1. 에디터에서 블루프린트가 제대로 할당되었는지 안전 검사
	if (!FireStormClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("FireStormClass is empty! Please assign it in the AnimBlueprint."));
		return;
	}m_pMTGSkillSlotC

	ACharacter* pPlayer = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

	// 2. 이 애니메이션을 실행 중인 몬스터(자신)를 가져옵니다.
	APawn* MonsterOwner = TryGetPawnOwner();

	if (IsValid(pPlayer) && IsValid(MonsterOwner))
	{
		FVector PlayerLocation = pPlayer->GetActorLocation();
		float CapsuleHalfHeight = pPlayer->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		FVector vImpactPoint = PlayerLocation - FVector(0.f, 0.f, CapsuleHalfHeight);

		FActorSpawnParameters params;
		params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		// 3. ★핵심: 이펙트를 깐 주인을 몬스터로 지정 (OnOverlapBegin에서 피아 식별용)
		params.Instigator = MonsterOwner;

		// 4. 경로를 적는 대신, 에디터에서 할당한 FireStormClass(블루프린트)를 스폰합니다.
		AEffect_Dot* Effect = GetWorld()->SpawnActor<AEffect_DoT>(
			FireStormClass,
			vImpactPoint,
			FRotator::ZeroRotator,
			params
		);

		// 5. 앞서 만든 초기화 함수 호출 (파티클, 사운드 재생)
		if (IsValid(Effect))
		{
			Effect->InitializeEffect();
		}
	}*/
//}
