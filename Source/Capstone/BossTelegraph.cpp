#include "BossTelegraph.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "CapstoneSFX.h"

ABossTelegraph::ABossTelegraph()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	OuterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OuterMesh"));
	OuterMesh->SetupAttachment(Root);

	FillMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FillMesh"));
	FillMesh->SetupAttachment(Root);

	for (UStaticMeshComponent* Mesh : { OuterMesh, FillMesh })
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetReceivesDecals(false);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderAsset.Succeeded())
	{
		OuterMesh->SetStaticMesh(CylinderAsset.Object);
		FillMesh->SetStaticMesh(CylinderAsset.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent"));
	if (MaterialAsset.Succeeded())
	{
		OuterMesh->SetMaterial(0, MaterialAsset.Object);
		FillMesh->SetMaterial(0, MaterialAsset.Object);
	}

	InitialLifeSpan = 10.f;
}

float ABossTelegraph::FindGroundZ(UWorld* World, const FVector& Location)
{
	// 캐릭터(플레이어/보스 캡슐, 메시)에 먼저 맞으면 예고 표시가 머리 높이에 떠버리므로 정적 지형만 대상으로 트레이스한다
	FHitResult Hit;
	FCollisionQueryParams Params(NAME_None, false);
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);

	const FVector Start = Location + FVector(0.f, 0.f, 150.f);
	const FVector End = Location - FVector(0.f, 0.f, 1500.f);

	if (World && World->LineTraceSingleByObjectType(Hit, Start, End, ObjectParams, Params))
		return Hit.ImpactPoint.Z;

	return Location.Z;
}

void ABossTelegraph::Setup(EBossTelegraphShape InShape, const FLinearColor& Color, float InFillDuration)
{
	Shape = InShape;
	BaseColor = Color;
	FillDuration = FMath::Max(InFillDuration, 0.05f);

	// 원형/링은 원기둥(반지름 50, 높이 100), 직사각형도 같은 원기둥을 쓰지 않고 큐브가 필요하지만
	// 큐브 에셋을 하나 더 로드하는 대신 Line은 CubeMesh를 런타임에 교체한다
	if (Shape == EBossTelegraphShape::Line)
	{
		if (UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
		{
			OuterMesh->SetStaticMesh(CubeMesh);
			FillMesh->SetStaticMesh(CubeMesh);
		}
	}

	OuterMID = OuterMesh->CreateAndSetMaterialInstanceDynamic(0);
	FillMID = FillMesh->CreateAndSetMaterialInstanceDynamic(0);

	ApplyProgress(0.f);

	// 예고 표시가 차오르는 시간에 경고음의 길이를 맞춘다(짧게 차오르면 빠르게, 길면 느리게). Ring 은 충격파 확산이라 경고음이 없다
	if (Shape != EBossTelegraphShape::Ring)
		CapstoneSFX::PlayAt(this, ESFX::Boss_Telegraph, GetActorLocation(), 1.f, FMath::Clamp(1.2f / FillDuration, 0.7f, 1.6f));
}

ABossTelegraph* ABossTelegraph::SpawnCircle(UWorld* World, const FVector& Center, float Radius, float FillDuration, const FLinearColor& Color)
{
	if (!World)
		return nullptr;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector Location(Center.X, Center.Y, FindGroundZ(World, Center) + 4.f);
	ABossTelegraph* Actor = World->SpawnActor<ABossTelegraph>(ABossTelegraph::StaticClass(), Location, FRotator::ZeroRotator, Params);

	if (Actor)
	{
		Actor->Radius = Radius;
		Actor->Setup(EBossTelegraphShape::Circle, Color, FillDuration);
	}

	return Actor;
}

ABossTelegraph* ABossTelegraph::SpawnLine(UWorld* World, const FVector& Start, const FVector& Direction, float Length, float Width, float FillDuration, const FLinearColor& Color)
{
	if (!World)
		return nullptr;

	FVector Dir = Direction;
	Dir.Z = 0.f;
	Dir = Dir.GetSafeNormal();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 액터 원점은 시작점, 회전은 진행 방향. 메시는 원점에서 전방으로 뻗도록 로컬 오프셋을 준다
	const FVector Location(Start.X, Start.Y, FindGroundZ(World, Start) + 4.f);
	ABossTelegraph* Actor = World->SpawnActor<ABossTelegraph>(ABossTelegraph::StaticClass(), Location, Dir.Rotation(), Params);

	if (Actor)
	{
		Actor->Length = Length;
		Actor->Width = Width;
		Actor->Setup(EBossTelegraphShape::Line, Color, FillDuration);
	}

	return Actor;
}

ABossTelegraph* ABossTelegraph::SpawnRing(UWorld* World, const FVector& Center, float MaxRadius, float Duration, const FLinearColor& Color)
{
	if (!World)
		return nullptr;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector Location(Center.X, Center.Y, FindGroundZ(World, Center) + 6.f);
	ABossTelegraph* Actor = World->SpawnActor<ABossTelegraph>(ABossTelegraph::StaticClass(), Location, FRotator::ZeroRotator, Params);

	if (Actor)
	{
		Actor->Radius = MaxRadius;
		Actor->LingerTime = 0.05f;
		Actor->Setup(EBossTelegraphShape::Ring, Color, Duration);
	}

	return Actor;
}

void ABossTelegraph::ApplyProgress(float Progress)
{
	Progress = FMath::Clamp(Progress, 0.f, 1.f);

	// 기본 원기둥은 반지름 50/높이 100, 큐브는 한 변 100. 두께는 항상 2cm 정도로 납작하게 둔다
	constexpr float ThicknessScale = 0.02f;

	switch (Shape)
	{
	case EBossTelegraphShape::Circle:
	{
		const float OuterScale = Radius / 50.f;
		OuterMesh->SetRelativeScale3D(FVector(OuterScale, OuterScale, ThicknessScale));

		const float FillScale = FMath::Max(OuterScale * Progress, 0.001f);
		FillMesh->SetRelativeScale3D(FVector(FillScale, FillScale, ThicknessScale * 1.5f));
		break;
	}
	case EBossTelegraphShape::Line:
	{
		// 큐브 중심이 원점이므로 길이의 절반만큼 앞으로 밀어준다. 채움은 시작점에 고정된 채 앞으로 자란다
		OuterMesh->SetRelativeScale3D(FVector(Length / 100.f, Width / 100.f, ThicknessScale));
		OuterMesh->SetRelativeLocation(FVector(Length * 0.5f, 0.f, 0.f));

		const float FilledLength = FMath::Max(Length * Progress, 0.1f);
		FillMesh->SetRelativeScale3D(FVector(FilledLength / 100.f, Width / 100.f, ThicknessScale * 1.5f));
		FillMesh->SetRelativeLocation(FVector(FilledLength * 0.5f, 0.f, 0.f));
		break;
	}
	case EBossTelegraphShape::Ring:
	{
		// 안쪽 원판이 퍼져나가고 외곽은 숨긴다
		const float FillScale = FMath::Max(Radius / 50.f * Progress, 0.001f);
		OuterMesh->SetRelativeScale3D(FVector(0.001f, 0.001f, 0.001f));
		FillMesh->SetRelativeScale3D(FVector(FillScale, FillScale, ThicknessScale));
		break;
	}
	}

	if (Shape == EBossTelegraphShape::Ring)
	{
		// 퍼지는 동안 점점 투명해진다
		FLinearColor RingColor = BaseColor;
		RingColor.A = BaseColor.A * (1.f - Progress * 0.9f);

		if (FillMID)
			FillMID->SetVectorParameterValue(TEXT("Color"), RingColor);
	}
	else
	{
		FLinearColor OuterColor = BaseColor;
		OuterColor.A = BaseColor.A * 0.35f;

		// 채워질수록 진해져서 "곧 터진다"는 긴박감을 준다
		FLinearColor FillColor = BaseColor;
		FillColor.A = FMath::Lerp(BaseColor.A * 0.7f, FMath::Min(BaseColor.A * 1.5f, 0.9f), Progress);

		if (OuterMID)
			OuterMID->SetVectorParameterValue(TEXT("Color"), OuterColor);
		if (FillMID)
			FillMID->SetVectorParameterValue(TEXT("Color"), FillColor);
	}
}

void ABossTelegraph::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Elapsed += DeltaTime;

	ApplyProgress(Elapsed / FillDuration);

	if (Elapsed >= FillDuration + LingerTime)
		Destroy();
}

void ABossTelegraph::Dismiss()
{
	Destroy();
}
