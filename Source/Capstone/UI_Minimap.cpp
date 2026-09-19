#include "UI_Minimap.h"
#include "Py_Grey.h"
#include "QuestComponent.h"
#include "PortalPoint.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/OverlaySlot.h"
#include "Engine/Texture2D.h"
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
}

void UUI_Minimap::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!OwnerPlayer.IsValid())
		return;

	FVector QuestTargetLocation;
	const bool bHasQuestTarget = ResolveQuestTargetLocation(QuestTargetLocation);
	UpdateDirectionalMarker(QuestMarkerIcon, bHasQuestTarget, QuestTargetLocation);

	// 포탈은 레벨 중간에 새로 생기거나 사라지지 않으므로(리스폰되는 몬스터와 다름) 딱 한 번만 찾는다
	if (!bPortalSearchAttempted)
	{
		CachedPortalTarget = FindNearestVillagePortal();
		bPortalSearchAttempted = true;
	}

	const bool bHasPortalTarget = CachedPortalTarget.IsValid();
	UpdateDirectionalMarker(PortalMarkerIcon, bHasPortalTarget,
		bHasPortalTarget ? CachedPortalTarget->GetActorLocation() : FVector::ZeroVector);
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
