#pragma once

#include "CoreMinimal.h"
#include "UI_QuestStyle.h"
#include "ItemData.h"
#include "EquipmentData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

// 상점/강화 창이 인벤토리/장비창과 같은 모양(골드 이중 테두리 판 + 제목줄 + 금화/아이콘 칸 + 글자 버튼)을 코드로 조립할 때 같이 쓰는 부품.
// 색/브러시 헬퍼는 UI_QuestStyle.h(QuestUI::)를 그대로 쓰고, 여기서는 그 위에 "창 한 벌"을 만드는 데 필요한 조각만 더한다
namespace PanelUI
{
	inline const FLinearColor Danger = QuestUI::SRGB(0xE0, 0x6C, 0x5E);   // 부족/실패

	// A쪽을 (1-T) 만큼, B쪽을 T 만큼 섞는다. 알파는 1로 둔다
	inline FLinearColor Blend(const FLinearColor& A, const FLinearColor& B, float T)
	{
		return FLinearColor(FMath::Lerp(A.R, B.R, T), FMath::Lerp(A.G, B.G, T), FMath::Lerp(A.B, B.B, T), 1.f);
	}

	// 1,234 처럼 세 자리마다 쉼표를 찍은 숫자
	inline FString Num(int32 Value)
	{
		return FText::AsNumber(Value).ToString();
	}

	// ---------------------------------------------------------------- 창 틀

	struct FPanelFrame
	{
		UVerticalBox* Column = nullptr;   // 제목줄 아래 내용을 위에서부터 쌓는 세로 상자
		UButton* CloseButton = nullptr;
	};

	// 바깥 프레임(어두운 판 + 금색 테두리) + 제목줄(마름모, 제목, 키 안내, 닫기 X) + 금색 구분선까지 만들고 Tree의 루트로 건다
	inline FPanelFrame BuildFrame(UWidgetTree* Tree, float ContentWidth, const TCHAR* Title, const TCHAR* KeyHint)
	{
		using namespace QuestUI;

		FPanelFrame Result;

		UBorder* Outer = MakeBorder(Tree, TEXT("Outer"), MakeBoxBrush(PanelFill, Gold, 2.f, 10.f), FMargin(3.f));
		Tree->RootWidget = Outer;

		UBorder* Inner = MakeBorder(Tree, TEXT("Inner"), MakeBoxBrush(PanelInner, GoldDim, 1.f, 8.f), FMargin(22.f, 16.f, 22.f, 18.f));
		Outer->SetContent(Inner);

		USizeBox* WidthBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("WidthBox"));
		WidthBox->SetWidthOverride(ContentWidth);
		Inner->SetContent(WidthBox);

		Result.Column = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
		WidthBox->SetContent(Result.Column);

		UHorizontalBox* Header = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Header"));

		if (UHorizontalBoxSlot* DiamondSlot = Header->AddChildToHorizontalBox(MakeDiamond(Tree, TEXT("TitleDiamond"), Gold, 9.f)))
		{
			DiamondSlot->SetVerticalAlignment(VAlign_Center);
			DiamondSlot->SetPadding(FMargin(4.f, 0.f, 10.f, 0.f));
		}

		UTextBlock* TitleText = MakeText(Tree, TEXT("Title"), 22, GoldBright, true, true, 6);
		TitleText->SetText(FText::FromString(Title));

		if (UHorizontalBoxSlot* TitleSlot = Header->AddChildToHorizontalBox(TitleText))
		{
			TitleSlot->SetVerticalAlignment(VAlign_Center);
			TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		UTextBlock* HintText = MakeText(Tree, TEXT("KeyHint"), 12, Muted, false);
		HintText->SetText(FText::FromString(KeyHint));

		if (UHorizontalBoxSlot* HintSlot = Header->AddChildToHorizontalBox(HintText))
		{
			HintSlot->SetVerticalAlignment(VAlign_Center);
			HintSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
		}

		Result.CloseButton = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
		Result.CloseButton->SetStyle(MakeButtonStyle(InsetFill, PanelInner, InsetFill, GoldDim, Gold));

		UTextBlock* CloseLabel = MakeText(Tree, TEXT("CloseLabel"), 14, Ivory, true);
		CloseLabel->SetText(FText::FromString(TEXT("X")));
		CloseLabel->SetJustification(ETextJustify::Center);
		Result.CloseButton->SetContent(CloseLabel);

		USizeBox* CloseBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CloseBox"));
		CloseBox->SetWidthOverride(28.f);
		CloseBox->SetHeightOverride(28.f);
		CloseBox->SetContent(Result.CloseButton);

		if (UHorizontalBoxSlot* CloseSlot = Header->AddChildToHorizontalBox(CloseBox))
			CloseSlot->SetVerticalAlignment(VAlign_Center);

		Result.Column->AddChildToVerticalBox(Header);

		if (UVerticalBoxSlot* LineSlot = Result.Column->AddChildToVerticalBox(MakeLine(Tree, TEXT("TitleLine"), Gold, 1.5f, true)))
			LineSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 14.f));

		return Result;
	}

	// ---------------------------------------------------------------- 소지금 / 버튼

	// 금색 원 + 숫자. OutText에 숫자 글자가 담긴다
	inline UHorizontalBox* MakeGoldBadge(UWidgetTree* Tree, const TCHAR* Name, int32 FontSize, UTextBlock*& OutText)
	{
		using namespace QuestUI;

		UHorizontalBox* Badge = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), FName(Name));

		const float CoinSize = FMath::Max(12.f, FontSize * 0.9f);

		USizeBox* CoinBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName(*FString::Printf(TEXT("%s_CoinBox"), Name)));
		CoinBox->SetWidthOverride(CoinSize);
		CoinBox->SetHeightOverride(CoinSize);
		CoinBox->SetContent(MakeBorder(Tree, *FString::Printf(TEXT("%s_Coin"), Name), MakeBoxBrush(Gold, GoldBright, 2.f, CoinSize * 0.5f)));

		if (UHorizontalBoxSlot* CoinSlot = Badge->AddChildToHorizontalBox(CoinBox))
		{
			CoinSlot->SetVerticalAlignment(VAlign_Center);
			CoinSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		}

		OutText = MakeText(Tree, *FString::Printf(TEXT("%s_Text"), Name), FontSize, GoldBright, true);

		if (UHorizontalBoxSlot* TextSlot = Badge->AddChildToHorizontalBox(OutText))
			TextSlot->SetVerticalAlignment(VAlign_Center);

		return Badge;
	}

	// 글자만 있는 버튼 (탭/수량 칩/큰 행동 버튼). Width/Height가 0이면 그 방향은 내용 크기를 따른다
	inline USizeBox* MakeTextButton(UWidgetTree* Tree, const TCHAR* Name, const TCHAR* Label, int32 FontSize, float Width, float Height, UButton*& OutButton, UTextBlock*& OutLabel)
	{
		using namespace QuestUI;

		USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName(*FString::Printf(TEXT("%s_Box"), Name)));

		if (Width > 0.f)
			Box->SetWidthOverride(Width);

		if (Height > 0.f)
			Box->SetHeightOverride(Height);

		OutButton = Tree->ConstructWidget<UButton>(UButton::StaticClass(), FName(Name));
		OutButton->SetStyle(MakeButtonStyle(InsetFill, PanelInner, InsetFill, GoldDim, Gold));

		OutLabel = MakeText(Tree, *FString::Printf(TEXT("%s_Label"), Name), FontSize, Muted, true, true, 1);
		OutLabel->SetText(FText::FromString(Label));
		OutLabel->SetJustification(ETextJustify::Center);
		OutButton->SetContent(OutLabel);

		Box->SetContent(OutButton);
		return Box;
	}

	// 탭/수량 칩: 선택된(bOn) 것은 금색으로 채우고, 아닌 것은 어둡게
	inline void ApplyToggleStyle(UButton* Button, UTextBlock* Label, bool bOn)
	{
		using namespace QuestUI;

		if (Button)
		{
			if (bOn)
				Button->SetStyle(MakeButtonStyle(SRGB(0x4A, 0x39, 0x18), SRGB(0x5A, 0x45, 0x1C), SRGB(0x3A, 0x2C, 0x12), Gold, GoldBright));
			else
				Button->SetStyle(MakeButtonStyle(InsetFill, PanelInner, InsetFill, GoldDim, Gold));
		}

		if (Label)
			Label->SetColorAndOpacity(FSlateColor(bOn ? GoldBright : Muted));
	}

	// 눈에 띄는 큰 행동 버튼(강화하기 등). 비활성일 때는 MakeButtonStyle이 알아서 어둡게 만든다
	inline void ApplyPrimaryStyle(UButton* Button, UTextBlock* Label)
	{
		using namespace QuestUI;

		if (Button)
			Button->SetStyle(MakeButtonStyle(SRGB(0x6B, 0x52, 0x1E), SRGB(0x85, 0x66, 0x26), SRGB(0x52, 0x3E, 0x16), Gold, GoldBright));

		if (Label)
			Label->SetColorAndOpacity(FSlateColor(GoldBright));
	}

	// ---------------------------------------------------------------- 아이템 아이콘 칸

	// 어두운 홈 + 아이템 색 테두리 + 아이콘. 아이콘이 없으면 이름 글자를 대신 보여준다
	struct FIconBox
	{
		TWeakObjectPtr<UBorder> Frame;
		TWeakObjectPtr<UImage> Icon;
		TWeakObjectPtr<UTextBlock> Fallback;

		// 테두리/바탕만 다시 칠한다. Glow(0~1)가 커지면 테두리가 흰빛으로 밝아진다 (강화 성공 연출)
		void Paint(const FLinearColor& Accent, float Glow = 0.f) const
		{
			if (UBorder* F = Frame.Get())
			{
				const FLinearColor Fill = Blend(QuestUI::SRGB(0x24, 0x1C, 0x12), Accent, 0.12f + 0.3f * Glow);
				const FLinearColor Outline = Blend(Accent, FLinearColor::White, 0.85f * Glow);
				F->SetBrush(QuestUI::MakeBoxBrush(Fill, Outline, 2.f + 2.f * Glow, 5.f));
			}
		}

		void Set(UTexture2D* Texture, const FText& FallbackName, const FLinearColor& Accent, float Glow = 0.f) const
		{
			Paint(Accent, Glow);

			if (UImage* I = Icon.Get())
			{
				if (Texture)
					I->SetBrushFromTexture(Texture);

				I->SetVisibility(Texture ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
			}

			if (UTextBlock* T = Fallback.Get())
			{
				T->SetText(FallbackName);
				T->SetVisibility(Texture ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
			}
		}
	};

	inline USizeBox* MakeIconBox(UWidgetTree* Tree, const TCHAR* Name, float Size, int32 FallbackFontSize, FIconBox& Out)
	{
		using namespace QuestUI;

		USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName(*FString::Printf(TEXT("%s_Box"), Name)));
		Box->SetWidthOverride(Size);
		Box->SetHeightOverride(Size);

		UBorder* Frame = MakeBorder(Tree, *FString::Printf(TEXT("%s_Frame"), Name), MakeBoxBrush(InsetFill, GoldDim, 2.f, 5.f), FMargin(5.f));
		Box->SetContent(Frame);
		Out.Frame = Frame;

		UOverlay* Layers = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), FName(*FString::Printf(TEXT("%s_Layers"), Name)));
		Frame->SetContent(Layers);

		UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), FName(*FString::Printf(TEXT("%s_Image"), Name)));
		Image->SetVisibility(ESlateVisibility::Hidden);
		Out.Icon = Image;

		if (UOverlaySlot* IconSlot = Layers->AddChildToOverlay(Image))
		{
			IconSlot->SetHorizontalAlignment(HAlign_Fill);
			IconSlot->SetVerticalAlignment(VAlign_Fill);
		}

		// 자동 줄바꿈 글자는 가운데 정렬 슬롯에서 임시 글자 폭에서 줄이 바뀌므로 Fill로 두고 글자만 가운데 정렬한다
		UTextBlock* Text = MakeText(Tree, *FString::Printf(TEXT("%s_Fallback"), Name), FallbackFontSize, Ivory, true);
		Text->SetJustification(ETextJustify::Center);
		Text->SetAutoWrapText(true);
		Text->SetVisibility(ESlateVisibility::Collapsed);
		Out.Fallback = Text;

		if (UOverlaySlot* TextSlot = Layers->AddChildToOverlay(Text))
		{
			TextSlot->SetHorizontalAlignment(HAlign_Fill);
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}

		return Box;
	}

	// ---------------------------------------------------------------- 아이템 정보

	// 장비이면 그 정보를 채워 true. DataTable 행에 장비 필드가 있으면 그것을, 없으면 코드 정의(EquipmentData.cpp)를 쓴다
	inline bool ResolveEquip(FName ItemID, const FItemData* Data, FEquipInfo& Out)
	{
		if (Data && Data->EquipSlot != EEquipSlot::None)
		{
			Out.Slot = Data->EquipSlot;
			Out.Name = Data->DisplayName;
			Out.AttackBonus = Data->AttackBonus;
			Out.MaxHpBonus = Data->MaxHpBonus;
			return true;
		}

		if (const FEquipInfo* Def = EquipDefs::FindDefinition(ItemID))
		{
			Out = *Def;
			return true;
		}

		return false;
	}

	// 아이템 종류별 테두리 색: 장비/무기는 금색, 물약은 붉은색, 재료는 푸른색, 전리품은 보라색
	inline FLinearColor ItemAccent(const FItemData* Data, bool bEquipment)
	{
		using namespace QuestUI;

		if (bEquipment)
			return Gold;

		if (!Data)
			return Muted;

		switch (Data->ItemType)
		{
		case EItemType::Potion:		return SRGB(0xD8, 0x64, 0x5C);
		case EItemType::Material:	return SRGB(0x6F, 0xA8, 0xD8);
		case EItemType::Loot:		return SRGB(0xAE, 0x88, 0xDC);
		default:					return Gold;
		}
	}

	inline FText ItemTypeLabel(const FItemData* Data)
	{
		if (!Data)
			return FText::GetEmpty();

		switch (Data->ItemType)
		{
		case EItemType::Potion:		return FText::FromString(TEXT("소모품"));
		case EItemType::Material:	return FText::FromString(TEXT("재료"));
		case EItemType::Loot:		return FText::FromString(TEXT("전리품"));
		default:					return FText::FromString(TEXT("무기"));
		}
	}

	// 상점 한 줄의 작은 설명: "소모품  ·  체력 50 회복" / "장비  ·  무기  ·  공격력 +5"
	inline FString ItemSummary(FName ItemID, const FItemData* Data)
	{
		FEquipInfo Equip;

		if (ResolveEquip(ItemID, Data, Equip))
		{
			FString Text = FString::Printf(TEXT("장비  ·  %s"), *EquipDefs::GetSlotLabel(Equip.Slot).ToString());

			if (Equip.AttackBonus != 0)
				Text += FString::Printf(TEXT("  ·  공격력 %+d"), Equip.AttackBonus);

			if (Equip.MaxHpBonus != 0)
				Text += FString::Printf(TEXT("  ·  체력 %+d"), Equip.MaxHpBonus);

			return Text;
		}

		FString Text = ItemTypeLabel(Data).ToString();

		if (Data && Data->HealAmount > 0)
			Text += FString::Printf(TEXT("  ·  체력 %d 회복"), Data->HealAmount);

		return Text;
	}

	// DataTable에 지정된 아이콘, 없으면 코드로 그린 검/방패 아이콘, 그것도 없으면 nullptr
	inline UTexture2D* LoadItemIcon(FName ItemID, const FItemData* Data)
	{
		UTexture2D* Texture = nullptr;

		if (Data && !Data->Icon.IsNull())
			Texture = Data->Icon.LoadSynchronous();

		if (!Texture)
			Texture = EquipDefs::GetGeneratedIcon(ItemID);

		return Texture;
	}
}
