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

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UInventoryComponent* InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UQuestComponent* QuestComponent;

	class UPy_GreyAnim* pGreyAnim;
protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void SetInteractableItem(class AItemBase* Item) { CurrItem = Item; }
	void SetInteractableNPC(class ANPC_Base* NPC) { CurrNPC = NPC; }

	
	virtual void SetExplosiveBuffState(bool bIsActive) override;



private:
	UFUNCTION()
	void RefreshQuestTracker();

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
