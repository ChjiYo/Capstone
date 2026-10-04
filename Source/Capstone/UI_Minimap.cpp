#include "UI_Minimap.h"
#include "Py_Grey.h"
#include "QuestComponent.h"
#include "PortalPoint.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/OverlaySlot.h"
#include "Engine/Texture2D.h"
#include "MinimapBaker.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"
#include "EngineUtils.h"

UTexture2D* UUI_Minimap::CreateTriangleTexture(int32 Size)
{
	UTexture2D* Texture = UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8);
	if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0)
		return nullptr;

	Texture->SRGB = true;
	Texture->Filter = TextureFilter::TF_Bilinear;
	Texture->AddressX = TextureAddress::TA_Clamp;
	Texture->AddressY = TextureAddress::TA_Clamp;
	Texture->LODGroup = TEXTUREGROUP_UI;
	Texture->NeverStream = true;

	// 위쪽을 향한 이등변삼각형 (0~1 정규화 좌표, y는 아래로 증가). 가장자리 여백을 조금 둬서 잘리지 않게 한다
	const FVector2D Top(0.5, 0.06);
	const FVector2D BottomLeft(0.10, 0.94);
	const FVector2D BottomRight(0.90, 0.94);

	auto EdgeSign = [](const FVector2D& P, const FVector2D& A, const FVector2D& B)
	{
		return (P.X - B.X) * (A.Y - B.Y) - (A.X - B.X) * (P.Y - B.Y);
	};

	// 픽셀당 4x4 슈퍼샘플링으로 삼각형 안에 든 비율을 알파로 써서 가장자리를 부드럽게 만든다
	constexpr int32 SubSamples = 4;
	FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
	uint8* Pixels = static_cast<uint8*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
	for (int32 Y = 0; Y < Size; ++Y)
	{
		for (int32 X = 0; X < Size; ++X)
		{
			int32 Inside = 0;
			for (int32 SY = 0; SY < SubSamples; ++SY)
			{
				for (int32 SX = 0; SX < SubSamples; ++SX)
				{
					const FVector2D P((X + (SX + 0.5) / SubSamples) / Size, (Y + (SY + 0.5) / SubSamples) / Size);
					const double D1 = EdgeSign(P, Top, BottomLeft);
					const double D2 = EdgeSign(P, BottomLeft, BottomRight);
					const double D3 = EdgeSign(P, BottomRight, Top);
					const bool bHasNeg = (D1 < 0) || (D2 < 0) || (D3 < 0);
					const bool bHasPos = (D1 > 0) || (D2 > 0) || (D3 > 0);
					if (!(bHasNeg && bHasPos))
						++Inside;
				}
			}

			const uint8 Alpha = static_cast<uint8>((Inside * 255) / (SubSamples * SubSamples));
			uint8* Pixel = Pixels + (Y * Size + X) * 4; // BGRA
			Pixel[0] = 255;
			Pixel[1] = 255;
			Pixel[2] = 255;
			Pixel[3] = Alpha;
		}
	}
	Mip.BulkData.Unlock();
	Texture->UpdateResource();

	return Texture;
}

UTexture2D* UUI_Minimap::CreateCircleMaskTexture(int32 Size, float RingThicknessPx, const FLinearColor& RingColor, const FLinearColor& OutsideColor)
{
	UTexture2D* Texture = UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8);
	if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0)
		return nullptr;

	Texture->SRGB = true;
	Texture->Filter = TextureFilter::TF_Bilinear;
	Texture->AddressX = TextureAddress::TA_Clamp;
	Texture->AddressY = TextureAddress::TA_Clamp;
	Texture->LODGroup = TEXTUREGROUP_UI;
	Texture->NeverStream = true;

	const FColor RingColor8 = RingColor.ToFColor(true);
	const FColor OutsideColor8 = OutsideColor.ToFColor(true);

	// 원 안쪽은 완전 투명(배경 캡처가 그대로 보임), 테두리는 불투명 골드 링, 그 바깥은 불투명 배경색으로
	// 네모난 캡처 텍스처의 모서리를 가린다. 삼각형 텍스처와 같은 4x4 슈퍼샘플링으로 두 경계 모두 부드럽게 만든다
	const float Radius = Size * 0.5f;
	const float RingInner = FMath::Max(Radius - RingThicknessPx, 0.f);

	constexpr int32 SubSamples = 4;
	FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
	uint8* Pixels = static_cast<uint8*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
	for (int32 Y = 0; Y < Size; ++Y)
	{
		for (int32 X = 0; X < Size; ++X)
		{
			int32 InsideCount = 0;
			int32 RingCount = 0;
			int32 OutsideCount = 0;

			for (int32 SY = 0; SY < SubSamples; ++SY)
			{
				for (int32 SX = 0; SX < SubSamples; ++SX)
				{
					const float PX = X + (SX + 0.5f) / SubSamples;
					const float PY = Y + (SY + 0.5f) / SubSamples;
					const float Dist = FVector2D(PX - Radius, PY - Radius).Size();

					if (Dist <= RingInner)
						++InsideCount;
					else if (Dist <= Radius)
						++RingCount;
					else
						++OutsideCount;
				}
			}

			const int32 TotalSamples = SubSamples * SubSamples;
			const float RingWeight = static_cast<float>(RingCount) / TotalSamples;
			const float OutsideWeight = static_cast<float>(OutsideCount) / TotalSamples;

			uint8* Pixel = Pixels + (Y * Size + X) * 4; // BGRA
			Pixel[0] = static_cast<uint8>(RingColor8.B * RingWeight + OutsideColor8.B * OutsideWeight);
			Pixel[1] = static_cast<uint8>(RingColor8.G * RingWeight + OutsideColor8.G * OutsideWeight);
			Pixel[2] = static_cast<uint8>(RingColor8.R * RingWeight + OutsideColor8.R * OutsideWeight);
			Pixel[3] = static_cast<uint8>(255.f * (RingWeight + OutsideWeight));
		}
	}
	Mip.BulkData.Unlock();
	Texture->UpdateResource();

	return Texture;
}

void UUI_Minimap::ApplyTriangleBrush(UImage* Icon, const FLinearColor& Color)
{
	if (!Icon || !TriangleTexture)
		return;

	// WBP에 남아 있던 텍스처/DrawAs/Tint에 영향받지 않도록 브러시를 통째로 새로 구성한다.
	// 색은 Brush Tint 대신 Color and Opacity 하나로만 낸다 (둘이 곱해져서 색이 어두워지는 것을 방지)
	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageType = ESlateBrushImageType::FullColor;
	Brush.ImageSize = FVector2D(TriangleTextureSize, TriangleTextureSize);
	Brush.TintColor = FLinearColor::White;
	Brush.SetResourceObject(TriangleTexture);

	Icon->SetBrush(Brush);
	Icon->SetColorAndOpacity(Color);
}

void UUI_Minimap::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// BindWidgetOptional은 이 시점에 이미 해석돼 있으므로 MarkerCanvas를 바로 써도 된다.
	// WBP를 건드리지 않고 배경/마스크를 코드로 끼워 넣어야, 에디터를 열지 않고도(현재처럼) 바로 확인할 수 있다
	BuildMapBackground();
}

void UUI_Minimap::BuildMapBackground()
{
	if (!bUseGeneratedMapBackground || !MarkerCanvas || NativeMapBackground)
		return;

	const FVector2D MapSize(MinimapRadius * 2.f, MinimapRadius * 2.f);

	auto CenterInCanvas = [](UWidget* Widget, const FVector2D& Size)
	{
		UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot);
		if (!Slot)
			return;

		Slot->SetAnchors(FAnchors(0.5f, 0.5f));
		Slot->SetAlignment(FVector2D(0.5f, 0.5f));
		Slot->SetAutoSize(true);
		Slot->SetPosition(FVector2D::ZeroVector);

		if (UImage* Image = Cast<UImage>(Widget))
		{
			Image->SetDesiredSizeOverride(Size);
		}
	};

	// 인덱스 0 = 맨 아래(배경), 1 = 그 위(원형 마스크). 기존에 WBP에서 바인딩된 QuestMarkerIcon/PortalMarkerIcon은
	// 이미 들어있던 자식이라 인덱스가 밀려 올라갈 뿐, 항상 배경/마스크보다 위(나중에 그려짐)에 남는다.
	// 배경은 회전하므로 같은 크기의 자르는 상자(NativeMapClip)에 넣어 미니맵 사각형 밖으로 나가지 않게 한다
	NativeMapClip = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("NativeMapClip"));
	NativeMapClip->SetWidthOverride(MapSize.X);
	NativeMapClip->SetHeightOverride(MapSize.Y);
	NativeMapClip->SetClipping(EWidgetClipping::ClipToBounds);
	NativeMapClip->SetVisibility(ESlateVisibility::HitTestInvisible);
	MarkerCanvas->InsertChildAt(0, NativeMapClip);
	CenterInCanvas(NativeMapClip, MapSize);

	NativeMapBackground = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("NativeMapBackground"));
	NativeMapClip->SetContent(NativeMapBackground);
	NativeMapBackground->SetDesiredSizeOverride(MapSize);

	// 지도는 UMinimapBaker가 첫 굽기를 끝낸 뒤 UpdateMapBackground가 연결한다.
	// 그때까지는 빈 사각형 대신 마스크의 바깥쪽 색으로 채워 둔다
	FSlateBrush PlaceholderBrush;
	PlaceholderBrush.DrawAs = ESlateBrushDrawType::Image;
	PlaceholderBrush.TintColor = MapMaskOutsideColor;
	NativeMapBackground->SetBrush(PlaceholderBrush);

	if (!CircleMaskTexture)
	{
		CircleMaskTexture = CreateCircleMaskTexture(MapMaskTextureSize, MapMaskRingThickness, MapMaskRingColor, MapMaskOutsideColor);
	}

	NativeMapMask = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("NativeMapMask"));
	MarkerCanvas->InsertChildAt(1, NativeMapMask);
	CenterInCanvas(NativeMapMask, MapSize);
	NativeMapMask->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (CircleMaskTexture)
	{
		FSlateBrush MaskBrush;
		MaskBrush.DrawAs = ESlateBrushDrawType::Image;
		MaskBrush.ImageType = ESlateBrushImageType::FullColor;
		// 브러시 크기가 텍스처 크기(256)로 남으면 DesiredSizeOverride보다 커져서 링이 미니맵 틀 밖으로 삐져나온다
		MaskBrush.ImageSize = MapSize;
		MaskBrush.TintColor = FLinearColor::White;
		MaskBrush.SetResourceObject(CircleMaskTexture);
		NativeMapMask->SetBrush(MaskBrush);
	}
}

void UUI_Minimap::InitMinimap(APy_Grey* InOwnerPlayer)
{
	OwnerPlayer = InOwnerPlayer;

	// PlayerIcon/QuestMarkerIcon이 WBP에서 Overlay/Canvas 중 어디에 들어있든, 아래 좌표 계산이
	// 가정하는 "정중앙 앵커 + 고정 크기" 상태를 코드가 직접 강제한다. 에디터에서 앵커/정렬을
	// 깜빡 잘못 잡아도(그동안 몇 번 겪었듯) 항상 동일하게 동작하도록 하기 위함
	auto ForceCenteredIcon = [](UWidget* Widget, const FVector2D& Size)
	{
		if (!Widget)
			return;

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
		{
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			// 앵커를 바꿔도 예전 오프셋(위치/크기)은 남는다. Size To Content가 꺼져 있으면 슬롯 크기(기본 100x100)가
			// 쓰여서 아래 DesiredSizeOverride가 무시되므로, 크기는 이미지 쪽에 맡기고 위치는 정중앙으로 되돌린다.
			// (방향 마커는 이후 매 Tick 위치를 다시 덮어쓴다)
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetPosition(FVector2D::ZeroVector);
		}
		else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(Widget->Slot))
		{
			OverlaySlot->SetHorizontalAlignment(HAlign_Center);
			OverlaySlot->SetVerticalAlignment(VAlign_Center);
		}

		if (UImage* Image = Cast<UImage>(Widget))
		{
			Image->SetDesiredSizeOverride(Size);
		}
	};

	ForceCenteredIcon(PlayerIcon, PlayerIconSize);
	ForceCenteredIcon(QuestMarkerIcon, QuestMarkerIconSize);
	ForceCenteredIcon(PortalMarkerIcon, PortalMarkerIconSize);

	if (bUseGeneratedTriangleIcons)
	{
		if (!TriangleTexture)
		{
			TriangleTexture = CreateTriangleTexture(TriangleTextureSize);
		}

		ApplyTriangleBrush(PlayerIcon, PlayerIconColor);
	}

	// MarkerCanvas는 반대로 부모 영역(FrameOverlay/MinimapRoot)을 정확히 꽉 채워야 QuestMarkerIcon의
	// 중앙 앵커가 PlayerIcon과 같은 지점(미니맵 시각적 중심)을 가리킨다
	if (MarkerCanvas)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MarkerCanvas->Slot))
		{
			CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			CanvasSlot->SetOffsets(FMargin(0.f));
		}
		else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(MarkerCanvas->Slot))
		{
			OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
			OverlaySlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	if (QuestMarkerIcon)
	{
		QuestMarkerIcon->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (PortalMarkerIcon)
	{
		PortalMarkerIcon->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 간략 지도 굽기: 플레이어의 캡처 컴포넌트를 빌려 쓴다 (굽는 순간에만 찍고 평소에는 꺼져 있음)
	if (bUseGeneratedMapBackground && NativeMapBackground && InOwnerPlayer && InOwnerPlayer->GetMinimapCaptureComponent())
	{
		UMinimapBaker::FSettings BakeSettings;
		BakeSettings.Resolution = MapBakeResolution;
		BakeSettings.BakeWorldSize = MapBakeWorldSize;
		// 다시 굽는 데 1초 남짓 걸리므로, 그동안 달려도 보이는 반경이 구운 영역 밖으로 나가지 않을 만큼 여유(25m)를 둔다
		BakeSettings.RebakeDistance = FMath::Max(MapBakeWorldSize * 0.5f - MinimapWorldRadius - 2500.f, 500.f);

		MapBaker = NewObject<UMinimapBaker>(this);
		MapBaker->Init(InOwnerPlayer->GetMinimapCaptureComponent(), InOwnerPlayer, BakeSettings);
	}
}

void UUI_Minimap::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!OwnerPlayer.IsValid())
		return;

	UpdateMapBackground();

	FVector QuestTargetLocation;
	const bool bHasQuestTarget = ResolveQuestTargetLocation(QuestTargetLocation);
	UpdateDirectionalMarker(QuestMarkerIcon, bHasQuestTarget, QuestTargetLocation);

	// 포탈은 레벨 중간에 새로 생기거나 사라지지 않으므로(리스폰되는 몬스터와 다름) 딱 한 번만 찾는다
	if (!bPortalSearchAttempted)
	{
		CachedPortalTarget = FindNearestVillagePortal();
		bPortalSearchAttempted = true;
	}

	// 퀘스트 마커가 떠 있으면(목표 자체든, 목표 레벨로 가는 포탈이든) 그것만 보여준다.
	// 돌아가는 길 마커는 퀘스트 안내가 없을 때만 (목표 레벨에 도착하면 퀘스트 위치에만 마커가 남는다)
	const bool bHasPortalTarget = !bHasQuestTarget && CachedPortalTarget.IsValid();
	UpdateDirectionalMarker(PortalMarkerIcon, bHasPortalTarget,
		bHasPortalTarget ? CachedPortalTarget->GetActorLocation() : FVector::ZeroVector);
}

void UUI_Minimap::UpdateMapBackground()
{
	if (!NativeMapBackground || !MapBaker)
		return;

	const FVector PlayerLoc = OwnerPlayer->GetActorLocation();
	MapBaker->Tick(PlayerLoc);

	UTexture2D* MapTexture = MapBaker->GetMapTexture();
	if (!MapTexture)
		return;

	// 구운 지도에서 플레이어 주변(보이는 반경)만 잘라 보여준다. 지도가 다시 구워지면 중심이 바뀌므로 매 틱 다시 계산
	const FVector2D CenterUV = MapBaker->WorldToUV(PlayerLoc);
	const float HalfUV = MinimapWorldRadius / FMath::Max(MapBaker->GetMapWorldSize(), 1.f);

	FSlateBrush Brush = NativeMapBackground->GetBrush();
	if (ShownMapTexture != MapTexture)
	{
		Brush = FSlateBrush();
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.ImageType = ESlateBrushImageType::FullColor;
		Brush.ImageSize = FVector2D(MinimapRadius * 2.f, MinimapRadius * 2.f);
		Brush.TintColor = FLinearColor::White;
		Brush.SetResourceObject(MapTexture);
		ShownMapTexture = MapTexture;
	}
	Brush.SetUVRegion(FBox2f(
		FVector2f(CenterUV.X - HalfUV, CenterUV.Y - HalfUV),
		FVector2f(CenterUV.X + HalfUV, CenterUV.Y + HalfUV)));
	NativeMapBackground->SetBrush(Brush);

	// 지도는 항상 정북(월드 +X가 위) 고정으로 구워지므로, 마커와 같은 "플레이어 진행방향이 항상 위" 착시는
	// 배경 이미지 자체를 플레이어 Yaw 반대로 돌려서 만든다. 마커들도 같은 관례(RelativeAngleDeg)로 위치를 계산한다
	FWidgetTransform BgTransform = NativeMapBackground->GetRenderTransform();
	BgTransform.Angle = -OwnerPlayer->GetActorRotation().Yaw + IconRotationOffsetDegrees;
	NativeMapBackground->SetRenderTransform(BgTransform);
}

void UUI_Minimap::UpdateDirectionalMarker(UImage* MarkerIcon, bool bHasTarget, const FVector& TargetLocation)
{
	if (!MarkerIcon)
		return;

	if (!bHasTarget || !MarkerCanvas)
	{
		MarkerIcon->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	MarkerIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	const FVector PlayerLoc = OwnerPlayer->GetActorLocation();
	const FVector2D WorldDelta2D(TargetLocation.X - PlayerLoc.X, TargetLocation.Y - PlayerLoc.Y);
	const float WorldDist = WorldDelta2D.Size();

	// 목표까지의 월드 방위각에서 플레이어가 지금 보는 방향(Yaw)을 빼서, "화면(정면=위쪽) 기준 상대각"으로 바꾼다.
	// 이렇게 하면 지도가 항상 플레이어 진행방향 기준으로 고정되어(플레이어-업 방식), 레벨이 어느 쪽을
	// "북쪽"으로 삼고 만들어졌는지와 무관하게 항상 정확하다 — 목표를 정면으로 바라보면 마커는 항상 정중앙 위쪽에 온다
	const float WorldBearingDeg = FMath::RadiansToDegrees(FMath::Atan2(WorldDelta2D.Y, WorldDelta2D.X));
	const float RelativeAngleDeg = FMath::UnwindDegrees(WorldBearingDeg - OwnerPlayer->GetActorRotation().Yaw + IconRotationOffsetDegrees);

	// 목표가 미니맵이 표현하는 반경보다 멀면 방향은 유지한 채 가장자리에 고정해 화살표처럼 보이게 한다
	const float ClampedWorldDist = FMath::Min(WorldDist, MinimapWorldRadius);
	const float MapDist = MinimapWorldRadius > KINDA_SMALL_NUMBER ? (ClampedWorldDist / MinimapWorldRadius) * MinimapRadius : 0.f;

	float SinAngle, CosAngle;
	FMath::SinCos(&SinAngle, &CosAngle, FMath::DegreesToRadians(RelativeAngleDeg));
	const FVector2D ScreenOffset(MapDist * SinAngle, -MapDist * CosAngle);

	if (UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(MarkerIcon->Slot))
	{
		MarkerSlot->SetPosition(ScreenOffset);
	}

	FWidgetTransform MarkerTransform = MarkerIcon->GetRenderTransform();
	MarkerTransform.Angle = RelativeAngleDeg;
	MarkerIcon->SetRenderTransform(MarkerTransform);
}

bool UUI_Minimap::ResolveQuestTargetLocation(FVector& OutLocation)
{
	if (!OwnerPlayer.IsValid() || !OwnerPlayer->QuestComponent)
		return false;

	const AActor* Target = OwnerPlayer->QuestComponent->GetQuestTargetActor();
	if (!Target)
		return false;

	OutLocation = Target->GetActorLocation();
	return true;
}

APortalPoint* UUI_Minimap::FindNearestVillagePortal() const
{
	UWorld* World = GetWorld();
	if (!World || !OwnerPlayer.IsValid())
		return nullptr;

	const FVector PlayerLoc = OwnerPlayer->GetActorLocation();

	APortalPoint* Nearest = nullptr;
	float BestDistSq = -1.f;
	int32 TotalPortals = 0;
	int32 FlaggedPortals = 0;
	for (TActorIterator<APortalPoint> It(World); It; ++It)
	{
		APortalPoint* Portal = *It;
		if (!IsValid(Portal))
			continue;

		++TotalPortals;
		if (!Portal->ShouldShowOnMinimap())
			continue;

		++FlaggedPortals;

		const float DistSq = FVector::DistSquared(PlayerLoc, Portal->GetActorLocation());
		if (!Nearest || DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Nearest = Portal;
		}
	}

	// 포탈 마커가 안 보일 때 원인을 바로 구분하기 위한 로그: 아이콘 바인딩 / 레벨 내 포탈 수 / 체크된 포탈 수
	LOG(TEXT("미니맵 포탈 탐색 (레벨=%s): 전체 포탈=%d, '미니맵에 표시' 체크된 포탈=%d, PortalMarkerIcon 바인딩=%s, MarkerCanvas 바인딩=%s"),
		*World->GetMapName(), TotalPortals, FlaggedPortals,
		PortalMarkerIcon ? TEXT("O") : TEXT("X"), MarkerCanvas ? TEXT("O") : TEXT("X"));

	return Nearest;
}
