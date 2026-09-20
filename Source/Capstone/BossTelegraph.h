#pragma once

#include "GameInfo.h"
#include "GameFramework/Actor.h"
#include "BossTelegraph.generated.h"

UENUM()
enum class EBossTelegraphShape : uint8
{
	Circle,		// 원형 경고 (안쪽 원이 차오름)
	Line,		// 직사각형 경고 (돌진 경로. 시작점에서 끝점 방향으로 차오름)
	Ring		// 퍼져나가는 충격파 (반투명 원판이 커지면서 옅어짐)
};

// 보스 공격 예고용 바닥 표시. 외곽(옅은 영역) + 안쪽(차오르는 영역) 두 겹의 납작한 메시로 구성되며,
// 채워지는 순간이 실제 타격 타이밍이라 플레이어가 회피 타이밍을 읽을 수 있다.
// 엔진 기본 메시/머티리얼만 사용하므로 별도 에셋 작업이 필요 없다
UCLASS()
class CAPSTONE_API ABossTelegraph : public AActor
{
	GENERATED_BODY()

public:
	ABossTelegraph();

	static ABossTelegraph* SpawnCircle(UWorld* World, const FVector& Center, float Radius, float FillDuration, const FLinearColor& Color);
	static ABossTelegraph* SpawnLine(UWorld* World, const FVector& Start, const FVector& Direction, float Length, float Width, float FillDuration, const FLinearColor& Color);
	static ABossTelegraph* SpawnRing(UWorld* World, const FVector& Center, float MaxRadius, float Duration, const FLinearColor& Color);

	// 예고가 끝나기 전에 스킬이 끊겼을 때 즉시 치운다
	void Dismiss();

	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY()
	USceneComponent* Root;

	UPROPERTY()
	UStaticMeshComponent* OuterMesh;

	UPROPERTY()
	UStaticMeshComponent* FillMesh;

	UPROPERTY()
	UMaterialInstanceDynamic* OuterMID = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* FillMID = nullptr;

	EBossTelegraphShape Shape = EBossTelegraphShape::Circle;
	FLinearColor BaseColor = FLinearColor::Red;

	float Elapsed = 0.f;
	float FillDuration = 1.f;

	// 채움이 끝난 뒤 잠깐 남겨 두었다가 사라지는 시간
	float LingerTime = 0.15f;

	// Circle/Ring은 반지름, Line은 길이/폭
	float Radius = 100.f;
	float Length = 100.f;
	float Width = 100.f;

	void Setup(EBossTelegraphShape InShape, const FLinearColor& Color, float InFillDuration);
	void ApplyProgress(float Progress);
	static float FindGroundZ(UWorld* World, const FVector& Location);
};
