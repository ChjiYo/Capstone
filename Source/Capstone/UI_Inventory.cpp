#include "UI_Inventory.h"
#include "InventoryComponent.h"
#include "UI_InventorySlot.h"
#include "UI_ItemDrag.h"
#include "UI_QuestStyle.h"
#include "EquipmentComponent.h"
#include "Py_Grey.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	// 격자: 한 줄에 5칸 (20칸 = 5x4). 칸마다 사방 3의 여백을 둬서 칸 사이가 6이 된다
	constexpr int32 GridColumns = 5;
	constexpr float SlotPadding = 3.f;
	constexpr float GridFramePadding = 7.f;
}

void UUI_Inventory::RefreshInventoryUI(UInventoryComponent* InInventoryComp)
{
	if (!InInventoryComp || !InventoryWrapBox) return;

	if (InventoryComp != InInventoryComp)
	{
		if (InventoryComp)
		{
			InventoryComp->OnInventoryChanged.RemoveDynamic(this, &UUI_Inventory::HandleInventoryChanged);
		}

		InventoryComp = InInventoryComp;
		InventoryComp->OnInventoryChanged.AddUniqueDynamic(this, &UUI_Inventory::HandleInventoryChanged);
	}

	for (UUI_InventorySlot* pSlot : SlotArray)
	{
		if (pSlot)
		{
			pSlot->ClearSlot();
		}
	}

	for (int32 i = 0; i < InventoryComp->Items.Num(); ++i)
	{
		if (i < SlotArray.Num())
		{
			if (SlotArray[i])
			{
				const FInventorySlot& InvenSlot = InventoryComp->Items[i];
				SlotArray[i]->InitSlot(InvenSlot, InventoryComp->GetItemData(InvenSlot.ItemID));
			}
		}
	}

	if (GoldText)
	{
		// 코드 조립 레이아웃은 금화 그림 옆에 숫자만 쓴다
		if (bUseCodeBuiltLayout)
			GoldText->SetText(FText::FromString(FString::Printf(TEXT("%s G"), *FText::AsNumber(InventoryComp->GetGold()).ToString())));
		else
			GoldText->SetText(FText::FromString(FString::Printf(TEXT("%d Gold"), InventoryComp->GetGold())));
	}

	if (CapacityText)
	{
		CapacityText->SetText(FText::FromString(FString::Printf(TEXT("소지품   %d / %d"), InventoryComp->Items.Num(), MaxInventorySlots)));
	}

	// 아이템을 쓰거나 장착해서 마우스 밑의 칸 내용이 바뀌었을 수 있다
	RefreshInfoPanel();
}

void UUI_Inventory::HandleInventoryChanged()
{
	RefreshInventoryUI(InventoryComp);
}

void UUI_Inventory::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 위젯 블루프린트의 트리를 통째로 코드 조립 트리로 교체한다. 디자이너 미리보기가 아니라 실제 인스턴스에서만
	if (bUseCodeBuiltLayout && WidgetTree && !IsDesignTime())
	{
		BuildNativeTree();
	}

	// 버튼 연결과 칸 생성은 한 번만 한다 (창을 열 때마다 불리는 NativeConstruct에서 하면 중복으로 연결된다)
	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UUI_Inventory::OnCloseButtonClicked);
	}

	CreateSlots();
}

void UUI_Inventory::PlaceInViewport()
{
	if (!bUseCodeBuiltLayout)
		return;

	// 장비창(화면 왼쪽 0.2)과 나란히 볼 수 있게 오른쪽에 놓는다. SetPositionInViewport는 앵커를 (0,0)으로 되돌리므로 쓰지 않는다
	SetAnchorsInViewport(FAnchors(0.8f, 0.5f));
	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
}

void UUI_Inventory::CreateSlots()
{
	if (!InventoryWrapBox)
		return;

	const TSubclassOf<UUI_InventorySlot> SlotClass = InventorySlotClass ? InventorySlotClass : TSubclassOf<UUI_InventorySlot>(UUI_InventorySlot::StaticClass());

	InventoryWrapBox->ClearChildren();
	SlotArray.Empty();

	for (int32 i = 0; i < MaxInventorySlots; ++i)
	{
		UUI_InventorySlot* NewSlot = CreateWidget<UUI_InventorySlot>(this, SlotClass);
		if (NewSlot)
		{
			NewSlot->ClearSlot();
			NewSlot->OnSlotHover.AddUObject(this, &UUI_Inventory::HandleSlotHover);

			if (UWrapBoxSlot* WrapSlot = InventoryWrapBox->AddChildToWrapBox(NewSlot))
			{
				if (bUseCodeBuiltLayout)
					WrapSlot->SetPadding(FMargin(SlotPadding));
			}

			SlotArray.Add(NewSlot);
		}
	}
}

void UUI_Inventory::BuildNativeTree()
{
	using namespace QuestUI;

	const float GridWidth = GridColumns * (UUI_InventorySlot::Size + SlotPadding * 2.f) + 2.f;
	const float ContentWidth = GridWidth + GridFramePadding * 2.f;

	// 바깥 프레임: 어두운 판 + 금색 테두리 (장비창과 같은 모양)
	UBorder* Outer = MakeBorder(WidgetTree, TEXT("Outer"), MakeBoxBrush(PanelFill, Gold, 2.f, 10.f), FMargin(3.f));
	WidgetTree->RootWidget = Outer;

	UBorder* Inner = MakeBorder(WidgetTree, TEXT("Inner"), MakeBoxBrush(PanelInner, GoldDim, 1.f, 8.f), FMargin(22.f, 16.f, 22.f, 18.f));
	Outer->SetContent(Inner);

	USizeBox* WidthBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("WidthBox"));
	WidthBox->SetWidthOverride(ContentWidth);
	Inner->SetContent(WidthBox);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	WidthBox->SetContent(Column);

	// ---- 제목줄: 마름모 + 제목 + 키 안내 + 닫기 버튼 ----
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Header"));

	if (UHorizontalBoxSlot* DiamondSlot = Header->AddChildToHorizontalBox(MakeDiamond(WidgetTree, TEXT("TitleDiamond"), Gold, 9.f)))
	{
		DiamondSlot->SetVerticalAlignment(VAlign_Center);
		DiamondSlot->SetPadding(FMargin(4.f, 0.f, 10.f, 0.f));
	}

	UTextBlock* Title = MakeText(WidgetTree, TEXT("Title"), 22, GoldBright, true, true, 6);
	Title->SetText(FText::FromString(TEXT("인 벤 토 리")));

	if (UHorizontalBoxSlot* TitleSlot = Header->AddChildToHorizontalBox(Title))
	{
		TitleSlot->SetVerticalAlignment(VAlign_Center);
		TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UTextBlock* KeyHint = MakeText(WidgetTree, TEXT("KeyHint"), 12, Muted, false);
	KeyHint->SetText(FText::FromString(TEXT("[I]")));

	if (UHorizontalBoxSlot* HintSlot = Header->AddChildToHorizontalBox(KeyHint))
	{
		HintSlot->SetVerticalAlignment(VAlign_Center);
		HintSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	}

	CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("NativeCloseButton"));
	CloseButton->SetStyle(MakeButtonStyle(InsetFill, PanelInner, InsetFill, GoldDim, Gold));

	UTextBlock* CloseLabel = MakeText(WidgetTree, TEXT("CloseLabel"), 14, Ivory, true);
	CloseLabel->SetText(FText::FromString(TEXT("X")));
	CloseLabel->SetJustification(ETextJustify::Center);
	CloseButton->SetContent(CloseLabel);

	USizeBox* CloseBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CloseBox"));
	CloseBox->SetWidthOverride(28.f);
	CloseBox->SetHeightOverride(28.f);
	CloseBox->SetContent(CloseButton);

	if (UHorizontalBoxSlot* CloseSlot = Header->AddChildToHorizontalBox(CloseBox))
		CloseSlot->SetVerticalAlignment(VAlign_Center);

	Column->AddChildToVerticalBox(Header);

	if (UVerticalBoxSlot* LineSlot = Column->AddChildToVerticalBox(MakeLine(WidgetTree, TEXT("TitleLine"), Gold, 1.5f, true)))
		LineSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 14.f));

	// ---- 칸 격자 ----
	UBorder* GridFrame = MakeBorder(WidgetTree, TEXT("GridFrame"), MakeBoxBrush(InsetFill, GoldDim, 1.5f, 6.f), FMargin(GridFramePadding));
	Column->AddChildToVerticalBox(GridFrame);

	USizeBox* GridBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("GridBox"));
	GridBox->SetWidthOverride(GridWidth);
	GridFrame->SetContent(GridBox);

	InventoryWrapBox = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("NativeInventoryWrapBox"));
	GridBox->SetContent(InventoryWrapBox);

	// ---- 아이템 설명 칸: 칸에 마우스를 올리면 이름/종류/효과가 나온다 ----
	UBorder* InfoFrame = MakeBorder(WidgetTree, TEXT("InfoFrame"), MakeBoxBrush(InsetFill, GoldDim, 1.5f, 6.f), FMargin(14.f, 10.f, 14.f, 10.f));

	if (UVerticalBoxSlot* InfoFrameSlot = Column->AddChildToVerticalBox(InfoFrame))
		InfoFrameSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));

	USizeBox* InfoBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InfoBox"));
	InfoBox->SetHeightOverride(132.f); // 내용이 바뀌어도 창 높이가 흔들리지 않게 고정
	InfoFrame->SetContent(InfoBox);

	UVerticalBox* InfoColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InfoColumn"));
	InfoBox->SetContent(InfoColumn);

	InfoNameText = MakeText(WidgetTree, TEXT("InfoName"), 18, GoldBright, true);
	InfoColumn->AddChildToVerticalBox(InfoNameText);

	InfoTypeText = MakeText(WidgetTree, TEXT("InfoType"), 12, Muted, false);

	if (UVerticalBoxSlot* TypeSlot = InfoColumn->AddChildToVerticalBox(InfoTypeText))
		TypeSlot->SetPadding(FMargin(0.f, 1.f, 0.f, 6.f));

	InfoBodyText = MakeText(WidgetTree, TEXT("InfoBody"), 13, Ivory, false);
	InfoBodyText->SetAutoWrapText(true);

	if (UVerticalBoxSlot* BodySlot = InfoColumn->AddChildToVerticalBox(InfoBodyText))
		BodySlot->SetHorizontalAlignment(HAlign_Fill);

	// ---- 아래줄: 소지품 수 / 소지금 ----
	if (UVerticalBoxSlot* FootLineSlot = Column->AddChildToVerticalBox(MakeLine(WidgetTree, TEXT("FootLine"), GoldDim, 1.f, true)))
		FootLineSlot->SetPadding(FMargin(0.f, 14.f, 0.f, 10.f));

	UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Footer"));

	CapacityText = MakeText(WidgetTree, TEXT("CapacityText"), 13, Muted, false, true, 1);

	if (UHorizontalBoxSlot* CapacitySlot = Footer->AddChildToHorizontalBox(CapacityText))
	{
		CapacitySlot->SetVerticalAlignment(VAlign_Center);
		CapacitySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	// 금화: 금색 원 + 밝은 테두리
	USizeBox* CoinBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CoinBox"));
	CoinBox->SetWidthOverride(16.f);
	CoinBox->SetHeightOverride(16.f);
	CoinBox->SetContent(MakeBorder(WidgetTree, TEXT("Coin"), MakeBoxBrush(Gold, GoldBright, 2.f, 8.f)));

	if (UHorizontalBoxSlot* CoinSlot = Footer->AddChildToHorizontalBox(CoinBox))
	{
		CoinSlot->SetVerticalAlignment(VAlign_Center);
		CoinSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	}

	GoldText = MakeText(WidgetTree, TEXT("NativeGoldText"), 18, GoldBright, true);

	if (UHorizontalBoxSlot* GoldSlot = Footer->AddChildToHorizontalBox(GoldText))
		GoldSlot->SetVerticalAlignment(VAlign_Center);

	Column->AddChildToVerticalBox(Footer);

	RefreshInfoPanel();
}

void UUI_Inventory::HandleSlotHover(UUI_InventorySlot* HoveredWidget, bool bEntered)
{
	if (bEntered)
	{
		HoveredSlot = HoveredWidget;
	}
	else if (HoveredSlot.Get() == HoveredWidget)
	{
		HoveredSlot.Reset();
	}

	RefreshInfoPanel();
}

void UUI_Inventory::RefreshInfoPanel()
{
	if (!InfoNameText || !InfoTypeText || !InfoBodyText)
		return;

	const UUI_InventorySlot* Hovered = HoveredSlot.Get();

	if (Hovered && Hovered->HasItem())
	{
		const FInventorySlotInfo& Info = Hovered->GetInfo();

		InfoNameText->SetText(Info.Name);
		InfoNameText->SetColorAndOpacity(FSlateColor(Info.Accent));
		InfoTypeText->SetText(Info.Type);
		InfoBodyText->SetText(Info.Body);
		InfoBodyText->SetColorAndOpacity(FSlateColor(QuestUI::Ivory));
	}
	else
	{
		InfoNameText->SetText(FText::FromString(TEXT("아이템 정보")));
		InfoNameText->SetColorAndOpacity(FSlateColor(QuestUI::Muted));
		InfoTypeText->SetText(FText::GetEmpty());
		InfoBodyText->SetText(FText::FromString(TEXT("칸에 마우스를 올리면 설명이 나옵니다.\n검과 방패는 장비창(U)으로 끌어다 놓아 장착합니다.")));
		InfoBodyText->SetColorAndOpacity(FSlateColor(QuestUI::Muted));
	}
}

void UUI_Inventory::NativeConstruct()
{
	Super::NativeConstruct();

	bOpenUI = true;

	// NativeDestruct(창 닫기)가 구독을 풀기 때문에 다시 열릴 때마다 구독을 복구한다.
	// RefreshInventoryUI는 같은 컴포넌트면 재구독을 건너뛰므로, 여기서 하지 않으면 두 번째 열기부터 실시간 갱신이 멈춘다
	if (InventoryComp)
	{
		InventoryComp->OnInventoryChanged.AddUniqueDynamic(this, &UUI_Inventory::HandleInventoryChanged);
		// 창이 닫혀 있는 동안 바뀐 내용(장비 해제로 들어온 아이템 등)을 열 때 바로 반영한다
		RefreshInventoryUI(InventoryComp);
	}

	HoveredSlot.Reset();
	RefreshInfoPanel();
}

void UUI_Inventory::NativeDestruct()
{
	if (InventoryComp)
	{
		InventoryComp->OnInventoryChanged.RemoveDynamic(this, &UUI_Inventory::HandleInventoryChanged);
	}

	// 창이 닫히면 칸의 마우스 나감 이벤트가 오지 않을 수 있으니 여기서 정리한다
	HoveredSlot.Reset();

	Super::NativeDestruct();
}

FReply UUI_Inventory::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{

	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		return FReply::Handled();
	}
	bOpenUI = false;

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

bool UUI_Inventory::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const UItemDragDropOperation* Operation = Cast<UItemDragDropOperation>(InOperation);

	if (!Operation || Operation->FromEquipSlot == EEquipSlot::None)
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);

	if (APy_Grey* Player = Cast<APy_Grey>(GetOwningPlayerPawn()))
	{
		if (Player->EquipmentComponent)
			Player->EquipmentComponent->Unequip(Operation->FromEquipSlot);
	}

	return true;
}

void UUI_Inventory::OnCloseButtonClicked()
{
	RemoveFromParent();

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC)
	{
		// 장비창이 함께 열려 있으면 입력 잠금/마우스 커서를 유지해야 하므로 플레이어가 두 창의 상태를 보고 정리한다
		if (APy_Grey* Player = Cast<APy_Grey>(PC->GetPawn()))
		{
			Player->RefreshPanelInputMode();
			return;
		}

		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
		// UI가 떠 있는 동안 마우스 클릭(NormalAttack 키) 등이 눌린 채로 걸려 있을 수 있어 초기화
		PC->FlushPressedKeys();
	}
}
