#pragma once

#include "CharBase.h"
#include "CombatInterface.h"
#include "Py_Grey.generated.h"


UCLASS()
class CAPSTONE_API APy_Grey : public ACharBase, public ICombatInterface
{
	GENERATED_BODY()

public:
	APy_Grey();


protected:
	UPROPERTY(Category = Camera, VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* pCamera;

	UPROPERTY(Category = Arm, VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* pArm;

	UPROPERTY(Category = Collision, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* WeaponBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUI_HpBar> PlayerHpBarClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUI_Inventory> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUI_QuestTracker> QuestTrackerWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUI_ExpBar> ExpBarWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|FX")
	TSubclassOf<class AEffectDestroy> HitEffectClass;

	UPROPERTY()
	class UUI_HpBar* PlayerHpBar;

	UPROPERTY()
	class AItemBase* CurrItem;

	UPROPERTY()
	class ANPC_Base* CurrNPC;

	UPROPERTY()
	UUI_Inventory* InventoryWidget = nullptr;

	UPROPERTY()
	class UUI_QuestTracker* QuestTrackerWidget = nullptr;

	UPROPERTY()
	class UUI_ExpBar* ExpBarWidget = nullptr;


private:
	UPROPERTY(EditDefaultsOnly, Category = "Dash|Animation")
	UAnimMontage* DashForwardMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Dash|Animation")
	UAnimMontage* DashBackwardMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Dash|Animation")
	UAnimMontage* DashLeftMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Dash|Animation")
	UAnimMontage* DashRightMontage;


private:
	bool bIsExplosiveBuffActive = false;
	bool bGameplayInputLocked = false;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UInventoryComponent* InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UQuestComponent* QuestComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UExperienceComponent* ExperienceComponent;

	// 레벨업 1회당 증가하는 체력/공격력. 체력은 최대치와 현재치에 동일하게 더해진다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	int32 HpIncreasePerLevel = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	int32 AttackIncreasePerLevel = 5;

	class UPy_GreyAnim* pGreyAnim;
protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void SetInteractableItem(class AItemBase* Item) { CurrItem = Item; }
	void SetInteractableNPC(class ANPC_Base* NPC) { CurrNPC = NPC; }

	
	virtual void SetExplosiveBuffState(bool bIsActive) override;

	// 인벤토리/상점 등 UI가 열려 있는 동안 공격·스킬 입력만 막는다 (이동은 계속 허용)
	void SetGameplayInputLocked(bool bLocked) { bGameplayInputLocked = bLocked; }

	// 특정 포션 아이템을 사용(회복 + 소모). 인벤토리 슬롯 클릭 등 UI에서도 호출할 수 있도록 public
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UsePotionItem(FName ItemID);



private:
	UFUNCTION()
	void RefreshQuestTracker();

	UFUNCTION()
	void RefreshExpBar();

	UFUNCTION()
	void HandleLevelUp(int32 NewLevel);

private:
	void MoveFront(float _fScale);
	void MoveSide(float _fScale);
	void RotationZ(float _fScale);
	void RotationX(float _fScale);

private:
	void JumpKey();
	void NormalAttack();
	void SkillSlotA();
	void SkillSlotB();
	void SkillSlotC();
	void OnInteractKeyPressed();
	void ToggleInventory();
	void UsePotion();

	void RollOrDash();

public:
	void EnableWeaponCollision(bool bEnable);

public:
	UFUNCTION()
	void  WepaonBeginOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent*
		OtherComp, int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void WeaponEndOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

protected:
	float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
};
