#pragma once

#include "GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "UI_Equipment.generated.h"

// 'U' 키로 여는 장비창. 위젯 블루프린트 없이 코드로 트리를 조립한다 (UUI_Death와 같은 방식).
// 무기/방패 칸, 현재 공격력·체력(장비 보너스 포함)을 보여주고, 인벤토리와 아이템을 주고받는다
UCLASS()
class CAPSTONE_API UUI_Equipment : public UUserWidget
{
	GENERATED_BODY()

public:
	// 이 플레이어의 장비/스탯을 보여주도록 연결한다. 위젯을 뷰포트에 올리기 전에 호출
	void InitEquipment(class APy_Grey* InPlayer);

	// 뷰포트에 올린 직후 호출: 화면 왼쪽 가운데쯤에 놓는다 (인벤토리 창과 나란히 볼 수 있게)
	void PlaceInViewport();

	// 닫기 버튼(X)을 눌렀을 때. 플레이어가 창을 닫고 입력 모드를 정리한다
	DECLARE_MULTICAST_DELEGATE(FOnCloseRequested);
	FOnCloseRequested OnCloseRequested;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildTree();
	void RefreshSlots();
	void RefreshStats(bool bForce);

	UFUNCTION()
	void HandleEquipmentChanged();

	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY()
	TObjectPtr<class APy_Grey> Player;

	UPROPERTY()
	TObjectPtr<class UUI_EquipSlot> WeaponSlotWidget;

	UPROPERTY()
	TObjectPtr<class UUI_EquipSlot> ShieldSlotWidget;

	UPROPERTY()
	TObjectPtr<class UButton> CloseButton;

	UPROPERTY()
	TObjectPtr<class UTextBlock> AttackValueText;

	UPROPERTY()
	TObjectPtr<class UTextBlock> AttackDetailText;

	UPROPERTY()
	TObjectPtr<class UTextBlock> HpValueText;

	UPROPERTY()
	TObjectPtr<class UTextBlock> HpDetailText;

	// 마지막으로 그린 값. 같으면 텍스트를 다시 쓰지 않는다 (NativeTick이 매 프레임 확인)
	int32 ShownAttack = -1;
	int32 ShownEffectiveAttack = -1;
	int32 ShownAttackBonus = -1;
	int32 ShownHp = -1;
	int32 ShownMaxHp = -1;
	int32 ShownHpBonus = -1;
};
