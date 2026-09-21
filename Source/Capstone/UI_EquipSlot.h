#pragma once

#include "GameInfo.h"
#include "ItemData.h"
#include "Blueprint/UserWidget.h"
#include "UI_EquipSlot.generated.h"

// 장비창의 칸 하나(무기 또는 방패). 위젯 블루프린트 없이 코드로 트리를 조립한다.
// 칸을 클릭하면 장비를 벗고, 인벤토리 아이템을 끌어다 놓으면 장착한다 (칸을 끌어 인벤토리 창에 놓아도 벗는다)
UCLASS()
class CAPSTONE_API UUI_EquipSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetEquipSlot(EEquipSlot InSlot);
	EEquipSlot GetEquipSlot() const { return EquipSlot; }

	// 지금 장착 상태를 아이콘/이름/보너스 텍스트에 반영한다
	void Refresh(class UEquipmentComponent* Equipment);

protected:
	virtual void NativeOnInitialized() override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	void BuildTree();

	// 테두리 색을 지금 상태(기본/마우스 오버/끌어다 놓을 수 있음)에 맞춘다
	void UpdateFrame();

	class UEquipmentComponent* GetEquipment() const;

	EEquipSlot EquipSlot = EEquipSlot::None;

	FName CurrentItemID;

	bool bHovered = false;

	// 끌고 있는 아이템이 이 칸에 맞을 때 true (맞는 칸이 초록으로 빛난다)
	bool bDropHighlight = false;

	bool bPressedOnItem = false;

	UPROPERTY()
	TObjectPtr<class UBorder> Frame;

	UPROPERTY()
	TObjectPtr<class UImage> IconImage;

	UPROPERTY()
	TObjectPtr<class UTextBlock> SlotLabelText;

	UPROPERTY()
	TObjectPtr<class UTextBlock> EmptyText;

	UPROPERTY()
	TObjectPtr<class UTextBlock> NameText;

	UPROPERTY()
	TObjectPtr<class UTextBlock> BonusText;
};
