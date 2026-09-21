#pragma once

#include "CoreMinimal.h"

class UTexture2D;

// 이미지 에셋 없이 아이콘을 코드로 그리는 작은 도구. 다각형 여러 장을 안티에일리어싱해서 겹쳐 그린다.
// 좌표는 아이콘 한가운데가 (0,0), 가장자리가 ±0.5이고 y가 위쪽이다. 나중에 추가한 모양이 앞에 그려진다
namespace IconPainter
{
	struct FShape
	{
		TArray<FVector2D> Points;
		FColor Fill = FColor::White;
		FColor Outline = FColor::Black;

		// 다각형 바깥쪽으로 이 두께만큼 테두리를 두른다 (0이면 테두리 없음)
		float OutlineWidth = 0.f;
	};

	FShape MakeRect(float CX, float CY, float W, float H, FColor Fill, FColor Outline, float OutlineWidth);
	FShape MakeCircle(float CX, float CY, float R, FColor Fill, FColor Outline, float OutlineWidth);
	FShape MakeEllipse(float CX, float CY, float RX, float RY, FColor Fill, FColor Outline, float OutlineWidth);
	FShape MakePoly(const TArray<FVector2D>& Points, FColor Fill, FColor Outline, float OutlineWidth);

	// 모든 모양을 (0,0)을 중심으로 시계 반대 방향(화면 기준)으로 돌린다
	void Rotate(TArray<FShape>& Shapes, float Degrees);

	// 모양들을 Size x Size 텍스처에 그린다. 가비지 컬렉션에 지워지지 않도록 루트에 붙여 두므로 호출하는 쪽이 한 번만 만들어 재사용해야 한다
	UTexture2D* Render(const TArray<FShape>& Shapes, int32 Size = 96);
}
