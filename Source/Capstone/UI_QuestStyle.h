#pragma once

#include "CoreMinimal.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

// 퀘스트 수락창(UUI_Quest)과 HUD 트래커(UUI_QuestTracker)가 코드로 위젯 트리를 조립할 때 같이 쓰는 색/브러시/텍스트 헬퍼.
// 팔레트는 Docs/12_story_ui.md의 대사창/챕터 카드와 같은 골드 + 아이보리 톤이다
namespace QuestUI
{
	// 색상 팔레트. 눈으로 보는 sRGB 값(#RRGGBB)으로 정하고 FromSRGBColor로 리니어로 바꾼다
	// (리니어 값을 그대로 쓰면 어두운 색이 훨씬 밝고 뿌옇게 나온다)
	inline FLinearColor SRGB(uint8 R, uint8 G, uint8 B, uint8 A = 255) { return FLinearColor::FromSRGBColor(FColor(R, G, B, A)); }

	inline const FLinearColor GoldBright = SRGB(0xF2, 0xD4, 0x88);    // 제목
	inline const FLinearColor Gold       = SRGB(0xCB, 0xA2, 0x4A);    // 라벨/테두리/강조
	inline const FLinearColor GoldDim    = SRGB(0xCB, 0xA2, 0x4A, 0x70); // 얇은 구분선/보조 테두리
	inline const FLinearColor Ivory      = SRGB(0xE8, 0xE2, 0xD0);    // 본문
	inline const FLinearColor Muted      = SRGB(0xA8, 0xA0, 0x8C);    // 보조 문구
	inline const FLinearColor Success    = SRGB(0x8C, 0xD8, 0x80);    // 완료
	inline const FLinearColor PanelFill  = SRGB(0x0E, 0x0C, 0x0A, 0xF5);
	inline const FLinearColor PanelInner = SRGB(0x1C, 0x17, 0x11, 0xFF);
	inline const FLinearColor InsetFill  = SRGB(0x0B, 0x09, 0x07, 0xD9);

	// 둥근 모서리 + 테두리가 있는 채움 브러시. UBorder/UButton/UProgressBar 어디에나 쓸 수 있다
	inline FSlateBrush MakeBoxBrush(const FLinearColor& Fill, const FLinearColor& Outline, float OutlineWidth, float Radius)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
		Brush.OutlineSettings.Color = FSlateColor(Outline);
		Brush.OutlineSettings.Width = OutlineWidth;
		return Brush;
	}

	inline UBorder* MakeBorder(UWidgetTree* Tree, const TCHAR* Name, const FSlateBrush& Brush, const FMargin& Padding = FMargin(0.f))
	{
		UBorder* Border = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(Name));
		Border->SetBrush(Brush);
		Border->SetPadding(Padding);
		return Border;
	}

	// 단색 가로/세로 선 (Fill 슬롯에 넣거나 SizeBox로 두께를 준다)
	inline USizeBox* MakeLine(UWidgetTree* Tree, const TCHAR* Name, const FLinearColor& Color, float Thickness, bool bHorizontal)
	{
		USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName(Name));
		if (bHorizontal)
			Box->SetHeightOverride(Thickness);
		else
			Box->SetWidthOverride(Thickness);

		Box->SetContent(MakeBorder(Tree, *FString::Printf(TEXT("%s_Fill"), Name), MakeBoxBrush(Color, FLinearColor::Transparent, 0.f, 0.f)));
		return Box;
	}

	// 45도 돌린 작은 마름모 장식. 폰트에 ◆ 글리프가 있는지에 의존하지 않으려고 도형으로 그린다
	inline USizeBox* MakeDiamond(UWidgetTree* Tree, const TCHAR* Name, const FLinearColor& Color, float Size)
	{
		USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName(Name));
		Box->SetWidthOverride(Size);
		Box->SetHeightOverride(Size);

		UBorder* Shape = MakeBorder(Tree, *FString::Printf(TEXT("%s_Shape"), Name), MakeBoxBrush(Color, FLinearColor::Transparent, 0.f, 0.f));
		Shape->SetRenderTransformAngle(45.f);
		Box->SetContent(Shape);
		return Box;
	}

	// Name은 WBP 트리에 이미 있는 위젯 이름과 겹치면 안 된다 (겹치면 나중에 SetText가 화면에 반영되지 않는다. UI_QuestTracker.cpp의 BuildNativeTree 주석 참고)
	inline UTextBlock* MakeText(UWidgetTree* Tree, const TCHAR* Name, int32 Size, const FLinearColor& Color, bool bBold, bool bShadow = true, int32 LetterSpacing = 0)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(Name));

		FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
		Font.LetterSpacing = LetterSpacing;

		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));

		if (bShadow)
		{
			Text->SetShadowOffset(FVector2D(1.f, 1.f));
			Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
		}

		return Text;
	}

	inline FButtonStyle MakeButtonStyle(const FLinearColor& Fill, const FLinearColor& HoverFill, const FLinearColor& PressFill, const FLinearColor& Outline, const FLinearColor& HoverOutline)
	{
		FButtonStyle Style;
		Style.Normal = MakeBoxBrush(Fill, Outline, 1.5f, 4.f);
		Style.Hovered = MakeBoxBrush(HoverFill, HoverOutline, 1.5f, 4.f);
		Style.Pressed = MakeBoxBrush(PressFill, HoverOutline, 1.5f, 4.f);
		Style.Disabled = MakeBoxBrush(Fill * 0.5f, Outline * 0.5f, 1.5f, 4.f);
		Style.NormalPadding = FMargin(0.f);
		Style.PressedPadding = FMargin(0.f, 1.f, 0.f, -1.f);
		return Style;
	}
}
