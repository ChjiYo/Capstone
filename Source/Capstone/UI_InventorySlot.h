#pragma once

#include "GameInfo.h"
#include "InventoryComponent.h"
#include "Blueprint/UserWidget.h"
#include "UI_InventorySlot.generated.h"

// 슬롯에 마우스를 올렸을 때 인벤토리 창 아래쪽 설명 칸에 보여줄 내용
struct FInventorySlotInfo
{
	FText Name;
	FText Type;
	FText Body;
	FLinearColor Accent = FLinearColor::White;
};

// 인벤토리 한 칸.
// bUseCodeBuiltLayout가 켜져 있으면(기본) 위젯 블루프린트(WBP_InventorySlot)의 트리를 버리고
// NativeOnInitialized에서 RPG풍 칸(어두운 홈 + 아이템 종류별 테두리 색 + 마우스 올리면 밝아지는 테두리)을 코드로 조립한다.
// 이때도 ItemIcon/CountText 바인딩은 그대로 채워진다. 끄면 예전처럼 WBP 레이아웃을 쓴다
UCLASS()
class CAPSTONE_API UUI_InventorySlot : public UUserWidget
{
	GENERATED_BODY()

public:
	// 칸 한 변의 길이. 인벤토리 창이 한 줄에 몇 칸이 들어가는지 계산할 때 쓴다
	static constexpr float Size = 64.f;

	void InitSlot(const FInventorySlot& SlotData, const FItemData* ItemData);
	void ClearSlot();

	bool HasItem() const { return !CurrentItemID.IsNone(); }
	const FInventorySlotInfo& GetInfo() const { return Info; }

	// 마우스가 칸에 들어오거나(true) 나갈 때(false) 방송. 인벤토리 창이 설명 칸을 갱신한다
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSlotHover, UUI_InventorySlot*, bool);
	FOnSlotHover OnSlotHover;

protected:
	virtual void NativeOnInitialized() override;

	// 슬롯을 클릭하면 포션류 아이템을 즉시 사용(회복+소모)한다. 장비(검/방패)는 클릭하면 장착하고, 끌어서 장비창에 놓아도 장착한다
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

	// 이 슬롯의 아이템이 장비이면 true (플레이어의 장비 컴포넌트에 물어본다)
	bool IsEquippableItem() const;

	// 장비 슬롯을 누른 채 아직 끌기/클릭이 갈리지 않은 상태
	bool bPressedOnEquipment = false;

	// 켜면 WBP 트리 대신 코드로 조립한 칸을 쓴다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Layout")
	bool bUseCodeBuiltLayout = true;

	UPROPERTY(meta = (BindWidget))
	class UImage* ItemIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* CountText;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FName CurrentItemID;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	int32 CurrentItemCount = 0;

private:
	void BuildNativeTree();

	// 아이템 유무/종류/마우스 위치에 맞게 테두리와 바탕색을 다시 칠한다
	void UpdateFrame();

	// 아이콘이 없는 아이템이 칸 안에 이름 글자를 대신 보여줄 때 쓴다
	void SetIconTexture(class UTexture2D* Texture, const FText& FallbackName);

	FInventorySlotInfo BuildInfo(const FItemData* ItemData) const;

	UPROPERTY(Transient)
	TObjectPtr<class UBorder> Frame;

	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> FallbackText;

	FInventorySlotInfo Info;

	bool bHovered = false;
};
