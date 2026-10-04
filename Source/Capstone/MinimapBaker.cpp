#include "MinimapBaker.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "RHIGPUReadback.h"
#include "RenderingThread.h"
#include "TextureResource.h"
#include "Async/Async.h"
#include "Async/ParallelFor.h"
#include "NavigationSystem.h"
#include "NavMesh/RecastNavMesh.h"
#include <atomic>

struct UMinimapBaker::FBakeJob
{
	int32 N = 0;
	FVector2D Center = FVector2D::ZeroVector;
	float CamZ = 0.f;
	float PlayerZ = 0.f;
	float WorldSize = 0.f;
	float FarPlane = 0.f;
	bool bIndoor = false;

	// 백그라운드 변환에 걸린 시간 (로그용)
	float StylizeMs = 0.f;

	TUniquePtr<FRHIGPUTextureReadback> Readback;

	// RGB = 조명 없는 색(리니어), A = 카메라에서의 깊이(cm)
	TArray<FLinearColor> Raw;

	// 구울 영역 안 내비메시 다각형들 (월드 좌표). NavPolyStart[i]~NavPolyStart[i+1]이 i번째 다각형의 꼭짓점
	TArray<FVector3f> NavVerts;
	TArray<int32> NavPolyStart;

	// 완성된 지도 (BGRA, sRGB)
	TArray<FColor> Pixels;

	std::atomic<bool> bPollQueued{ false };
	std::atomic<bool> bCopied{ false };
	std::atomic<bool> bStylizeStarted{ false };
	std::atomic<bool> bStylized{ false };
};

namespace MinimapStylize
{
	// 지도에 칠하는 면의 종류
	enum EMapClass : uint8
	{
		Void,		// 아무것도 안 찍힌 곳 (하늘/구운 영역 밖)
		Grass,		// 걸을 수 있는 풀밭
		Dirt,		// 걸을 수 있는 흙/모래 (실내 바닥도 이 색)
		Road,		// 걸을 수 있는 포장길/돌바닥
		Roof,		// 붉은 지붕
		Tree,		// 나무
		Rock,		// 바위/건물 등 막힌 곳
		Wall,		// 실내의 벽
		GrassDim,	// 갈 수는 없지만 자연 지형인 풀밭 (울타리 밖 등)
		DirtDim,	// 갈 수는 없지만 자연 지형인 흙
		ClassCount
	};

	// 눈으로 보는 sRGB 값 (양피지 지도 느낌의 차분한 색)
	static const FColor Palette[ClassCount] =
	{
		FColor(34, 29, 22),
		FColor(160, 178, 112),
		FColor(212, 194, 148),
		FColor(230, 220, 194),
		FColor(176, 96, 74),
		FColor(92, 126, 78),
		FColor(122, 116, 106),
		FColor(66, 60, 54),
		FColor(136, 150, 100),
		FColor(176, 162, 128),
	};

	static const FColor OutlineColor(60, 48, 36);

	static bool IsWalkClass(uint8 C) { return C == Grass || C == Dirt || C == Road; }
	static bool IsSolidClass(uint8 C) { return C == Roof || C == Tree || C == Rock || C == Wall; }

	// 가장자리를 늘려 붙이는 분리형 상자 흐림 (반경 R). 가로는 줄마다, 세로는 열 묶음마다 위에서 아래로 훑어
	// 메모리를 순서대로 읽게 하고, 둘 다 여러 스레드로 나눠 돌린다
	static void BoxBlur(TArray<float>& Data, int32 N, int32 R)
	{
		TArray<float> Temp;
		Temp.SetNumUninitialized(N * N);
		const float Inv = 1.f / (2 * R + 1);

		ParallelFor(N, [&](int32 Y)
		{
			const float* Src = &Data[Y * N];
			float* Dst = &Temp[Y * N];
			float Acc = 0.f;
			for (int32 I = -R; I <= R; ++I)
				Acc += Src[FMath::Clamp(I, 0, N - 1)];
			for (int32 X = 0; X < N; ++X)
			{
				Dst[X] = Acc * Inv;
				Acc += Src[FMath::Min(X + R + 1, N - 1)] - Src[FMath::Max(X - R, 0)];
			}
		});

		constexpr int32 Chunk = 64;
		ParallelFor((N + Chunk - 1) / Chunk, [&](int32 ChunkIndex)
		{
			const int32 X0 = ChunkIndex * Chunk;
			const int32 X1 = FMath::Min(X0 + Chunk, N);
			float Acc[Chunk];
			for (int32 X = X0; X < X1; ++X)
			{
				float Sum = 0.f;
				for (int32 I = -R; I <= R; ++I)
					Sum += Temp[FMath::Clamp(I, 0, N - 1) * N + X];
				Acc[X - X0] = Sum;
			}
			for (int32 Y = 0; Y < N; ++Y)
			{
				const float* Add = &Temp[FMath::Min(Y + R + 1, N - 1) * N];
				const float* Sub = &Temp[FMath::Max(Y - R, 0) * N];
				float* Dst = &Data[Y * N];
				for (int32 X = X0; X < X1; ++X)
				{
					Dst[X] = Acc[X - X0] * Inv;
					Acc[X - X0] += Add[X] - Sub[X];
				}
			}
		});
	}

	// 0/1 마스크를 반경 R 상자로 흐린 값 (= 상자 안에서 마스크가 차지하는 비율)
	template <typename FPred>
	static void BoxFraction(int32 N, int32 R, FPred&& Pred, TArray<float>& Out)
	{
		Out.SetNumUninitialized(N * N);
		ParallelFor(N, [&](int32 Y)
		{
			for (int32 X = 0; X < N; ++X)
				Out[Y * N + X] = Pred(Y * N + X) ? 1.f : 0.f;
		});
		BoxBlur(Out, N, R);
	}

	// 4방향 연결 성분 번호 매기기. 반환값 = 성분 수, OutAreas[번호] = 넓이 (0번은 마스크 밖)
	template <typename FPred>
	static int32 LabelComponents(int32 N, FPred&& InMask, TArray<int32>& OutLabels, TArray<int32>& OutAreas)
	{
		OutLabels.Init(0, N * N);
		OutAreas.Reset();
		OutAreas.Add(0);

		TArray<int32> Queue;
		int32 Next = 0;
		for (int32 Start = 0; Start < N * N; ++Start)
		{
			if (OutLabels[Start] != 0 || !InMask(Start))
				continue;

			const int32 Label = ++Next;
			Queue.Reset();
			Queue.Add(Start);
			OutLabels[Start] = Label;
			for (int32 Head = 0; Head < Queue.Num(); ++Head)
			{
				const int32 P = Queue[Head];
				const int32 X = P % N, Y = P / N;
				const int32 Neighbors[4] = { X > 0 ? P - 1 : -1, X < N - 1 ? P + 1 : -1, Y > 0 ? P - N : -1, Y < N - 1 ? P + N : -1 };
				for (int32 Q : Neighbors)
				{
					if (Q >= 0 && OutLabels[Q] == 0 && InMask(Q))
					{
						OutLabels[Q] = Label;
						Queue.Add(Q);
					}
				}
			}
			OutAreas.Add(Queue.Num());
		}
		return Next;
	}

	// 내비메시 다각형(볼록)을 마스크에 칠한다
	static void RasterizeNav(const UMinimapBaker::FBakeJob& Job, TArray<uint8>& OutMask)
	{
		const int32 N = Job.N;
		const float Size = FMath::Max(Job.WorldSize, 1.f);
		TArray<FVector2f> Poly;

		for (int32 PolyIndex = 0; PolyIndex + 1 < Job.NavPolyStart.Num(); ++PolyIndex)
		{
			Poly.Reset();
			FVector2f Min(FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX);
			for (int32 V = Job.NavPolyStart[PolyIndex]; V < Job.NavPolyStart[PolyIndex + 1]; ++V)
			{
				const FVector3f& W = Job.NavVerts[V];
				const FVector2f P((0.5f + (W.Y - Job.Center.Y) / Size) * N, (0.5f - (W.X - Job.Center.X) / Size) * N);
				Poly.Add(P);
				Min = FVector2f::Min(Min, P);
				Max = FVector2f::Max(Max, P);
			}
			if (Poly.Num() < 3)
				continue;

			float Area2 = 0.f;
			for (int32 I = 0; I < Poly.Num(); ++I)
				Area2 += FVector2f::CrossProduct(Poly[I], Poly[(I + 1) % Poly.Num()]);
			const float Orient = Area2 >= 0.f ? 1.f : -1.f;

			const int32 X0 = FMath::Max(FMath::FloorToInt(Min.X), 0), X1 = FMath::Min(FMath::CeilToInt(Max.X), N - 1);
			const int32 Y0 = FMath::Max(FMath::FloorToInt(Min.Y), 0), Y1 = FMath::Min(FMath::CeilToInt(Max.Y), N - 1);
			for (int32 Y = Y0; Y <= Y1; ++Y)
			{
				for (int32 X = X0; X <= X1; ++X)
				{
					const FVector2f C(X + 0.5f, Y + 0.5f);
					bool bInside = true;
					for (int32 I = 0; I < Poly.Num() && bInside; ++I)
					{
						const FVector2f& A = Poly[I];
						const FVector2f& B = Poly[(I + 1) % Poly.Num()];
						bInside = FVector2f::CrossProduct(B - A, C - A) * Orient >= -0.01f;
					}
					if (bInside)
						OutMask[Y * N + X] = 1;
				}
			}
		}
	}

	// 마스크에서 플레이어(중앙)가 속한 덩어리와, 그 절반 이상 큰 덩어리만 남긴다 (바위 속 같은 못 가는 섬 제거)
	static void KeepReachable(int32 N, TArray<uint8>& Mask)
	{
		TArray<int32> Labels, Areas;
		const int32 Count = LabelComponents(N, [&](int32 P) { return Mask[P] != 0; }, Labels, Areas);
		if (Count == 0)
			return;

		int32 Seed = 0;
		for (int32 R = 0; R <= 400 && Seed == 0; R += 4)
		{
			TMap<int32, int32> Votes;
			for (int32 Y = FMath::Max(N / 2 - R, 0); Y <= FMath::Min(N / 2 + R, N - 1); ++Y)
				for (int32 X = FMath::Max(N / 2 - R, 0); X <= FMath::Min(N / 2 + R, N - 1); ++X)
					if (const int32 L = Labels[Y * N + X])
						++Votes.FindOrAdd(L);

			int32 BestVotes = 0;
			for (const TPair<int32, int32>& Vote : Votes)
			{
				if (Vote.Value > BestVotes)
				{
					BestVotes = Vote.Value;
					Seed = Vote.Key;
				}
			}
		}
		if (Seed == 0)
			return;

		const int32 MinArea = Areas[Seed] / 2;
		for (int32 P = 0; P < N * N; ++P)
		{
			const int32 L = Labels[P];
			Mask[P] = (L != 0 && (L == Seed || Areas[L] >= MinArea)) ? 1 : 0;
		}
	}

	static void Run(UMinimapBaker::FBakeJob& Job)
	{
		const int32 N = Job.N;
		const int32 Count = N * N;
		Job.Pixels.SetNumUninitialized(Count);

		if (Job.Raw.Num() != Count)
		{
			for (FColor& P : Job.Pixels)
				P = Palette[Void];
			return;
		}

		const float PixelCm = Job.WorldSize / N;
		const float PixelArea = PixelCm * PixelCm;
		auto CmToPx = [&](float Cm) { return FMath::Max(FMath::RoundToInt(Cm / PixelCm), 1); };

		// 1) 높이와 빈 곳(하늘/원평면)
		TArray<float> Height;
		TArray<uint8> IsVoid;
		Height.SetNumUninitialized(Count);
		IsVoid.SetNumUninitialized(Count);
		for (int32 P = 0; P < Count; ++P)
		{
			const float Depth = Job.Raw[P].A;
			IsVoid[P] = (Depth <= 0.f || Depth >= Job.FarPlane * 0.999f) ? 1 : 0;
			Height[P] = Job.CamZ - Depth;
		}

		// 2) 재질 색(sRGB)을 흐려서 무늬를 지우고 색상/채도/명도로 바꾼다
		constexpr int32 GammaSteps = 4096;
		float Gamma[GammaSteps];
		for (int32 I = 0; I < GammaSteps; ++I)
			Gamma[I] = FMath::Pow(I / float(GammaSteps - 1), 1.f / 2.2f);

		TArray<float> Chan[3];
		for (int32 C = 0; C < 3; ++C)
		{
			Chan[C].SetNumUninitialized(Count);
			ParallelFor(N, [&](int32 Y)
			{
				for (int32 P = Y * N; P < (Y + 1) * N; ++P)
				{
					const FLinearColor& L = Job.Raw[P];
					const float V = C == 0 ? L.R : (C == 1 ? L.G : L.B);
					Chan[C][P] = Gamma[FMath::Clamp(FMath::RoundToInt(V * (GammaSteps - 1)), 0, GammaSteps - 1)];
				}
			});
			BoxBlur(Chan[C], N, 3);
			BoxBlur(Chan[C], N, 3);
		}

		TArray<float> Hue, Sat, Val;
		Hue.SetNumUninitialized(Count);
		Sat.SetNumUninitialized(Count);
		Val.SetNumUninitialized(Count);
		for (int32 P = 0; P < Count; ++P)
		{
			const float R = Chan[0][P], G = Chan[1][P], B = Chan[2][P];
			const float Mx = FMath::Max3(R, G, B), Mn = FMath::Min3(R, G, B);
			const float D = FMath::Max(Mx - Mn, 1e-5f);
			float H = 0.f;
			if (Mx == R)		H = FMath::Fmod((G - B) / D + 6.f, 6.f);
			else if (Mx == G)	H = (B - R) / D + 2.f;
			else				H = (R - G) / D + 4.f;
			Hue[P] = H * 60.f;
			Sat[P] = Mx > 1e-4f ? (Mx - Mn) / Mx : 0.f;
			Val[P] = Mx;
		}
		for (TArray<float>& C : Chan)
			C.Empty();

		auto IsGreen = [&](int32 P) { return Hue[P] > 55.f && Hue[P] < 170.f && Sat[P] > 0.15f; };
		auto IsRoof = [&](int32 P) { return (Hue[P] <= 28.f || Hue[P] >= 330.f) && Sat[P] > 0.2f && Val[P] > 0.3f; };

		// 3) 걸을 수 있는 바닥: 내비메시가 있으면 그것을, 없으면 높이(경사/턱)로 추정
		TArray<uint8> Walk;
		Walk.Init(0, Count);
		const bool bHasNav = Job.NavPolyStart.Num() > 1;
		if (bHasNav)
		{
			TArray<uint8> NavMask;
			NavMask.Init(0, Count);
			RasterizeNav(Job, NavMask);

			// 내비메시는 캐릭터 반경만큼 벽에서 떨어져 있으므로 그만큼 넓혀 실제 바닥 모양에 맞춘다
			TArray<float> Frac;
			BoxFraction(N, CmToPx(35.f), [&](int32 P) { return NavMask[P] != 0; }, Frac);
			for (int32 P = 0; P < Count; ++P)
				Walk[P] = Frac[P] > 1e-4f ? 1 : 0;
		}
		else
		{
			TArray<float> Smooth = Height;
			BoxBlur(Smooth, N, 3);
			for (int32 Y = 0; Y < N; ++Y)
			{
				for (int32 X = 0; X < N; ++X)
				{
					const int32 P = Y * N + X;
					const float GX = (Smooth[Y * N + FMath::Min(X + 1, N - 1)] - Smooth[Y * N + FMath::Max(X - 1, 0)]) / (2.f * PixelCm);
					const float GY = (Smooth[FMath::Min(Y + 1, N - 1) * N + X] - Smooth[FMath::Max(Y - 1, 0) * N + X]) / (2.f * PixelCm);
					// 45도보다 완만한 곳
					Walk[P] = (GX * GX + GY * GY < 1.f && !IsVoid[P]) ? 1 : 0;
				}
			}
			// 이웃과 60cm 넘게 차이 나는 턱(벽/지붕 끝)은 양쪽 모두 끊는다
			TArray<uint8> Barrier;
			Barrier.Init(0, Count);
			for (int32 Y = 0; Y < N; ++Y)
			{
				for (int32 X = 0; X < N; ++X)
				{
					const int32 P = Y * N + X;
					if (X + 1 < N && FMath::Abs(Height[P] - Height[P + 1]) > 60.f) { Barrier[P] = 1; Barrier[P + 1] = 1; }
					if (Y + 1 < N && FMath::Abs(Height[P] - Height[P + N]) > 60.f) { Barrier[P] = 1; Barrier[P + N] = 1; }
				}
			}
			for (int32 P = 0; P < Count; ++P)
				Walk[P] &= !Barrier[P];
		}

		for (int32 P = 0; P < Count; ++P)
			Walk[P] &= !IsVoid[P];

		KeepReachable(N, Walk);

		// 경계를 살짝 다듬는다 (반경 2 다수결)
		{
			TArray<float> Frac;
			BoxFraction(N, 2, [&](int32 P) { return Walk[P] != 0; }, Frac);
			for (int32 P = 0; P < Count; ++P)
				Walk[P] = Frac[P] > 0.5f ? 1 : 0;
		}

		// 높이로만 추정했을 때는 완만한 바위 비탈도 걸을 수 있는 곳으로 잡히므로, 바위색(차가운 회색) 큰 덩어리는 뺀다
		if (!bHasNav)
		{
			auto IsRocky = [&](int32 P)
			{
				return Walk[P] && Hue[P] > 170.f && Hue[P] < 260.f && Sat[P] > 0.04f && Sat[P] < 0.22f && Val[P] > 0.38f;
			};
			TArray<int32> Labels, Areas;
			LabelComponents(N, IsRocky, Labels, Areas);
			const int32 MinArea = FMath::RoundToInt(40e4f / PixelArea);
			for (int32 P = 0; P < Count; ++P)
			{
				if (Labels[P] != 0 && Areas[Labels[P]] >= MinArea)
					Walk[P] = 0;
			}
		}

		// 4) 면 종류
		TArray<uint8> Cls;
		Cls.SetNumUninitialized(Count);
		for (int32 P = 0; P < Count; ++P)
		{
			uint8 C = Dirt;
			if (IsVoid[P])
			{
				C = Void;
			}
			else if (Job.bIndoor)
			{
				C = Walk[P] ? Dirt : Wall;
			}
			else if (Walk[P])
			{
				if (IsGreen(P))								C = Grass;
				if (Sat[P] < 0.18f && Val[P] > 0.40f)		C = Road;
			}
			else
			{
				const bool bEarthy = Sat[P] > 0.17f && !IsRoof(P);
				C = Rock;
				if (bEarthy)							C = IsGreen(P) ? GrassDim : DirtDim;
				if (IsGreen(P) && Val[P] < 0.30f)		C = Tree;
				if (IsRoof(P))							C = Roof;
			}
			Cls[P] = C;
		}

		// 다수결 필터로 자잘한 얼룩을 지운다. 걸을 수 있는/없는 경계는 위에서 정한 그대로 둔다
		{
			TArray<uint8> BestClass;
			TArray<float> BestFrac, Frac;
			for (int32 Pass = 0; Pass < 3; ++Pass)
			{
				int32 Present[ClassCount] = {};
				for (int32 P = 0; P < Count; ++P)
					++Present[Cls[P]];

				BestClass.Init(0, Count);
				BestFrac.Init(-1.f, Count);
				for (uint8 K = 0; K < ClassCount; ++K)
				{
					if (Present[K] == 0)
						continue;

					BoxFraction(N, 3, [&](int32 P) { return Cls[P] == K; }, Frac);
					ParallelFor(N, [&](int32 Y)
					{
						for (int32 P = Y * N; P < (Y + 1) * N; ++P)
						{
							if (Frac[P] > BestFrac[P])
							{
								BestFrac[P] = Frac[P];
								BestClass[P] = K;
							}
						}
					});
				}
				for (int32 P = 0; P < Count; ++P)
				{
					if ((Walk[P] != 0) == IsWalkClass(BestClass[P]))
						Cls[P] = BestClass[P];
				}
			}
		}

		// 바위/지붕 사이에 낀 자잘한 "자연 지형" 조각은 바위로 (60㎡ 미만)
		{
			TArray<int32> Labels, Areas;
			LabelComponents(N, [&](int32 P) { return Cls[P] == GrassDim || Cls[P] == DirtDim; }, Labels, Areas);
			const int32 MinArea = FMath::RoundToInt(60e4f / PixelArea);
			for (int32 P = 0; P < Count; ++P)
			{
				if (Labels[P] != 0 && Areas[Labels[P]] < MinArea)
					Cls[P] = Rock;
			}
		}

		// 5) 칠하기: 면 색 → 막힌 곳이 왼쪽 위에서 드리우는 그림자 → 막힌 곳 안쪽 경계 → 바닥/막힌 곳 외곽선
		const int32 ShadowOffset = CmToPx(80.f);
		auto IsOpen = [&](int32 P) { return !IsSolidClass(Cls[P]) && Cls[P] != Void; };

		TArray<uint8> Edge;
		Edge.Init(0, Count);
		for (int32 Y = 0; Y < N; ++Y)
		{
			for (int32 X = 0; X < N; ++X)
			{
				const int32 P = Y * N + X;
				if (X + 1 < N && IsOpen(P) != IsOpen(P + 1)) { Edge[P] = 1; Edge[P + 1] = 1; }
				if (Y + 1 < N && IsOpen(P) != IsOpen(P + N)) { Edge[P] = 1; Edge[P + N] = 1; }
			}
		}

		ParallelFor(N, [&](int32 Y)
		{
			for (int32 X = 0; X < N; ++X)
			{
				const int32 P = Y * N + X;
				const uint8 C = Cls[P];
				float R = Palette[C].R, G = Palette[C].G, B = Palette[C].B;
				float Shade = 1.f;

				if (IsOpen(P))
				{
					const int32 SX = X - ShadowOffset, SY = Y - ShadowOffset;
					if (SX >= 0 && SY >= 0 && IsSolidClass(Cls[SY * N + SX]))
						Shade = 0.78f;
				}
				else if (IsSolidClass(C))
				{
					// 지붕-바위처럼 종류가 바뀌거나 1.5m 넘게 높이가 끊기는 곳에 옅은 선
					auto Inner = [&](int32 Q)
					{
						return IsSolidClass(Cls[Q]) && (Cls[Q] != C || FMath::Abs(Height[Q] - Height[P]) > 150.f);
					};
					if ((X > 0 && Inner(P - 1)) || (Y > 0 && Inner(P - N)))
						Shade = 0.7f;
				}

				// 외곽선은 한 겹 더 두껍게 (미니맵에서 절반 정도로 줄여 보이므로)
				const bool bEdge = Edge[P] || (X > 0 && Edge[P - 1]) || (X + 1 < N && Edge[P + 1]) || (Y > 0 && Edge[P - N]) || (Y + 1 < N && Edge[P + N]);
				if (bEdge && C != Void)
				{
					R = OutlineColor.R; G = OutlineColor.G; B = OutlineColor.B;
					Shade = 1.f;
				}

				Job.Pixels[P] = FColor(
					static_cast<uint8>(FMath::Clamp(R * Shade, 0.f, 255.f)),
					static_cast<uint8>(FMath::Clamp(G * Shade, 0.f, 255.f)),
					static_cast<uint8>(FMath::Clamp(B * Shade, 0.f, 255.f)),
					255);
			}
		});
	}
}

void UMinimapBaker::Init(USceneCaptureComponent2D* InCapture, AActor* InPlayer, const FSettings& InSettings)
{
	Capture = InCapture;
	Player = InPlayer;
	Settings = InSettings;
	Settings.Resolution = FMath::Clamp(Settings.Resolution, 256, 4096);

	if (!InCapture)
		return;

	CaptureTarget = NewObject<UTextureRenderTarget2D>(this);
	CaptureTarget->RenderTargetFormat = RTF_RGBA32f;
	CaptureTarget->ClearColor = FLinearColor(0.f, 0.f, 0.f, 0.f);
	CaptureTarget->bAutoGenerateMips = false;
	CaptureTarget->InitAutoFormat(Settings.Resolution, Settings.Resolution);
	CaptureTarget->UpdateResourceImmediate(true);

	InCapture->bCaptureEveryFrame = false;
	InCapture->bCaptureOnMovement = false;
	InCapture->TextureTarget = CaptureTarget;

	// RGB = 조명 없는 색, A = 깊이. 조명을 끄면 밤/동굴처럼 어두운 레벨에서도 재질 원래 색이 그대로 찍힌다
	InCapture->CaptureSource = ESceneCaptureSource::SCS_SceneColorSceneDepth;
	InCapture->ProjectionType = ECameraProjectionMode::Orthographic;
	InCapture->OrthoWidth = Settings.BakeWorldSize;
	InCapture->bAutoCalculateOrthoPlanes = false;

	// 높은 곳에서 넓게 찍으면 메시/랜드스케이프가 거친 LOD로 그려지므로, 가끔 한 번뿐인 굽기는 가장 세밀하게 찍는다
	InCapture->LODDistanceFactor = 0.01f;

	if (InPlayer)
	{
		InCapture->HiddenActors.AddUnique(InPlayer);
	}

	FEngineShowFlags& Flags = InCapture->ShowFlags;
	Flags.SetLighting(false);
	Flags.SetFog(false);
	Flags.SetAtmosphere(false);
	Flags.SetVolumetricFog(false);
	Flags.SetCloud(false);
	Flags.SetSkeletalMeshes(false);
	Flags.SetParticles(false);
	Flags.SetTranslucency(false);
	Flags.SetInstancedGrass(false);
	Flags.SetDynamicShadows(false);
	Flags.SetAmbientOcclusion(false);
	Flags.SetGlobalIllumination(false);
	Flags.SetLumenGlobalIllumination(false);
	Flags.SetLumenReflections(false);
	Flags.SetReflectionEnvironment(false);
	Flags.SetScreenSpaceReflections(false);
	Flags.SetTemporalAA(false);
	Flags.SetAntiAliasing(false);
	Flags.SetMotionBlur(false);
	Flags.SetBloom(false);
	Flags.SetEyeAdaptation(false);
	Flags.SetPostProcessing(false);

	// 노출 1 고정 (자동 노출의 사전 노출 배율이 색에 곱해지지 않도록)
	InCapture->PostProcessBlendWeight = 1.f;
	InCapture->PostProcessSettings.bOverride_AutoExposureMethod = true;
	InCapture->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	InCapture->PostProcessSettings.bOverride_AutoExposureBias = true;
	InCapture->PostProcessSettings.AutoExposureBias = 0.f;
	InCapture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	InCapture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
}

void UMinimapBaker::BeginDestroy()
{
	// 렌더/백그라운드 스레드가 들고 있는 작업은 공유 포인터라 그쪽에서 끝나면 알아서 사라진다
	PendingJob.Reset();
	Super::BeginDestroy();
}

FVector2D UMinimapBaker::WorldToUV(const FVector& WorldLocation) const
{
	// 카메라가 정북(월드 +X)을 위로 두고 내려다보므로 텍스처 위쪽 = +X, 오른쪽 = +Y
	const float Size = FMath::Max(Settings.BakeWorldSize, 1.f);
	return FVector2D(
		0.5f + (WorldLocation.Y - MapCenter.Y) / Size,
		0.5f - (WorldLocation.X - MapCenter.X) / Size);
}

bool UMinimapBaker::LooksIndoor(const FBakeJob& Job)
{
	const int32 N = Job.N;
	if (Job.Raw.Num() != N * N || Job.WorldSize <= 0.f)
		return false;

	// 나무 한 그루/처마 하나 밑에 서 있다고 실내로 오판하지 않도록 플레이어 둘레 7m 안의 여러 곳을 본다
	const float PixelsPerCm = N / Job.WorldSize;
	const int32 Ring = FMath::Max(FMath::RoundToInt(700.f * PixelsPerCm), 1);
	const int32 Step = FMath::Max(Ring / 3, 1);
	int32 Samples = 0;
	int32 Covered = 0;
	for (int32 DY = -Ring; DY <= Ring; DY += Step)
	{
		for (int32 DX = -Ring; DX <= Ring; DX += Step)
		{
			const int32 X = N / 2 + DX;
			const int32 Y = N / 2 + DY;
			if (X < 0 || Y < 0 || X >= N || Y >= N)
				continue;

			const float Height = Job.CamZ - Job.Raw[Y * N + X].A;
			++Samples;
			if (Height > Job.PlayerZ + 400.f)
				++Covered;
		}
	}

	return Samples > 0 && Covered >= Samples * 0.75f;
}

void UMinimapBaker::Tick(const FVector& PlayerLocation)
{
	if (!Capture.IsValid() || !CaptureTarget)
		return;

	if (PendingJob.IsValid())
	{
		PollReadback();
		return;
	}

	bool bNeedBake = !bHasMap;
	if (bHasMap)
	{
		const float Dist = FVector2D::Distance(FVector2D(PlayerLocation), MapCenter);
		bNeedBake = Dist > Settings.RebakeDistance
			|| (bMapIndoor && FMath::Abs(PlayerLocation.Z - MapPlayerZ) > Settings.IndoorRebakeHeight);
	}

	// 굽기가 계속 실패해도 1초에 한 번 이상은 시도하지 않는다
	const double Now = FPlatformTime::Seconds();
	if (bNeedBake && Now - LastBakeStartTime > 1.0)
	{
		LastBakeStartTime = Now;
		StartBake(PlayerLocation, false);
	}
}

void UMinimapBaker::StartBake(const FVector& PlayerLocation, bool bIndoor)
{
	USceneCaptureComponent2D* C = Capture.Get();
	FTextureRenderTargetResource* RTResource = CaptureTarget->GameThread_GetRenderTargetResource();
	if (!C || !RTResource)
		return;

	TSharedPtr<FBakeJob, ESPMode::ThreadSafe> Job = MakeShared<FBakeJob, ESPMode::ThreadSafe>();
	Job->N = Settings.Resolution;
	Job->Center = FVector2D(PlayerLocation);
	Job->PlayerZ = PlayerLocation.Z;
	Job->WorldSize = Settings.BakeWorldSize;
	Job->bIndoor = bIndoor;
	Job->CamZ = PlayerLocation.Z + (bIndoor ? Settings.IndoorCutHeight : Settings.OutdoorCameraHeight);
	Job->Readback = MakeUnique<FRHIGPUTextureReadback>(TEXT("MinimapBakeReadback"));

	// 정사영 근평면은 카메라 위치(0)라서 실내에서는 카메라 위쪽(천장)이 잘려 나간다.
	// 엔진 기본 원평면은 사실상 무한대라 깊이 정밀도가 없으므로 지형이 들어올 만큼만 잡은 행렬을 직접 넣는다
	const float FarPlane = (Job->CamZ - Job->PlayerZ) + 20000.f;
	Job->FarPlane = FarPlane;
	const float Half = Settings.BakeWorldSize * 0.5f;
	C->bUseCustomProjectionMatrix = true;
	C->CustomProjectionMatrix = FReversedZOrthoMatrix(Half, Half, 1.f / FarPlane, 0.f);
	C->OrthoWidth = Settings.BakeWorldSize;
	C->SetWorldLocationAndRotation(FVector(Job->Center.X, Job->Center.Y, Job->CamZ), FRotator(-90.f, 0.f, 0.f));
	C->CaptureScene();

	ENQUEUE_RENDER_COMMAND(MinimapBakeCopy)([Job, RTResource](FRHICommandListImmediate& RHICmdList)
	{
		Job->Readback->EnqueueCopy(RHICmdList, RTResource->GetRenderTargetTexture());
	});

	GatherNavPolys(*Job);

	PendingJob = Job;

	LOG(TEXT("미니맵 지도 굽기 시작: 중심=(%.0f, %.0f) 실내=%s 카메라Z=%.0f 내비메시 다각형=%d"), Job->Center.X, Job->Center.Y,
		bIndoor ? TEXT("O") : TEXT("X"), Job->CamZ, FMath::Max(Job->NavPolyStart.Num() - 1, 0));
}

void UMinimapBaker::GatherNavPolys(FBakeJob& Job) const
{
	UWorld* World = GetWorld();
	UNavigationSystemV1* NavSys = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
	const ARecastNavMesh* NavMesh = NavSys ? Cast<ARecastNavMesh>(NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate)) : nullptr;
	if (!NavMesh)
		return;

	// 실내는 지금 층 근처만 (위층/아래층 통로가 겹쳐 그려지지 않게), 야외는 높이 제한 없이
	const float Half = Job.WorldSize * 0.5f;
	const float MinZ = Job.bIndoor ? Job.PlayerZ - 1500.f : -UE_BIG_NUMBER;
	const float MaxZ = Job.bIndoor ? Job.CamZ : UE_BIG_NUMBER;
	const FBox BakeBox(FVector(Job.Center.X - Half, Job.Center.Y - Half, MinZ), FVector(Job.Center.X + Half, Job.Center.Y + Half, MaxZ));

	Job.NavPolyStart.Add(0);

	TArray<FNavTileRef> Tiles;
	NavMesh->GetAllNavMeshTiles(Tiles);

	TArray<FNavPoly> Polys;
	TArray<FVector> Verts;
	for (const FNavTileRef& Tile : Tiles)
	{
		const FBox TileBounds = NavMesh->GetNavMeshTileBounds(Tile);
		if (!TileBounds.IsValid || !TileBounds.Intersect(BakeBox))
			continue;

		Polys.Reset();
		if (!NavMesh->GetPolysInTile(Tile, Polys))
			continue;

		for (const FNavPoly& Poly : Polys)
		{
			if (Poly.Center.Z < MinZ || Poly.Center.Z > MaxZ)
				continue;

			Verts.Reset();
			if (!NavMesh->GetPolyVerts(Poly.Ref, Verts) || Verts.Num() < 3)
				continue;

			for (const FVector& V : Verts)
				Job.NavVerts.Add(FVector3f(V));
			Job.NavPolyStart.Add(Job.NavVerts.Num());
		}
	}
}

void UMinimapBaker::PollReadback()
{
	TSharedPtr<FBakeJob, ESPMode::ThreadSafe> Job = PendingJob;

	if (Job->bStylized)
	{
		FinishBake();
		return;
	}

	if (Job->bCopied)
	{
		// 야외로 찍어 봤더니 머리 위가 막혀 있으면(동굴 천장 등) 천장을 잘라 다시 찍는다
		if (!Job->bIndoor && !Job->bStylizeStarted && LooksIndoor(*Job))
		{
			PendingJob.Reset();
			StartBake(FVector(Job->Center.X, Job->Center.Y, Job->PlayerZ), true);
			return;
		}

		if (!Job->bStylizeStarted.exchange(true))
		{
			Async(EAsyncExecution::ThreadPool, [Job]()
			{
				const double StartTime = FPlatformTime::Seconds();
				MinimapStylize::Run(*Job);
				Job->StylizeMs = static_cast<float>((FPlatformTime::Seconds() - StartTime) * 1000.0);
				Job->bStylized = true;
			});
		}
		return;
	}

	if (Job->bPollQueued.exchange(true))
		return;

	ENQUEUE_RENDER_COMMAND(MinimapBakePoll)([Job](FRHICommandListImmediate& RHICmdList)
	{
		if (Job->Readback->IsReady())
		{
			int32 RowPitch = 0;
			const FLinearColor* Src = static_cast<const FLinearColor*>(Job->Readback->Lock(RowPitch));
			if (Src)
			{
				Job->Raw.SetNumUninitialized(Job->N * Job->N);
				for (int32 Y = 0; Y < Job->N; ++Y)
				{
					FMemory::Memcpy(&Job->Raw[Y * Job->N], Src + static_cast<int64>(Y) * RowPitch, sizeof(FLinearColor) * Job->N);
				}
			}
			Job->Readback->Unlock();
			Job->bCopied = true;
		}
		Job->bPollQueued = false;
	});
}

void UMinimapBaker::FinishBake()
{
	TSharedPtr<FBakeJob, ESPMode::ThreadSafe> Job = PendingJob;
	PendingJob.Reset();

	const int32 N = Job->N;
	if (Job->Pixels.Num() != N * N)
		return;

	UTexture2D* Texture = UTexture2D::CreateTransient(N, N, PF_B8G8R8A8);
	if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0)
		return;

	Texture->SRGB = true;
	Texture->Filter = TextureFilter::TF_Trilinear;
	Texture->AddressX = TextureAddress::TA_Clamp;
	Texture->AddressY = TextureAddress::TA_Clamp;
	Texture->LODGroup = TEXTUREGROUP_UI;
	Texture->NeverStream = true;

	// 미니맵은 이 텍스처를 절반 정도로 줄여 보여주므로, 외곽선이 깨지지 않게 밉맵 몇 단계를 직접 만들어 둔다
	FTexturePlatformData* PlatformData = Texture->GetPlatformData();
	TArray<FColor> Level = MoveTemp(Job->Pixels);
	int32 Size = N;
	for (int32 MipIndex = 0; MipIndex < 4 && Size >= 64; ++MipIndex)
	{
		FTexture2DMipMap* Mip = nullptr;
		if (MipIndex == 0)
		{
			Mip = &PlatformData->Mips[0];
		}
		else
		{
			Mip = new FTexture2DMipMap(Size, Size, 1);
			PlatformData->Mips.Add(Mip);
		}

		Mip->BulkData.Lock(LOCK_READ_WRITE);
		void* Dest = Mip->BulkData.Realloc(static_cast<int64>(Size) * Size * sizeof(FColor));
		FMemory::Memcpy(Dest, Level.GetData(), static_cast<int64>(Size) * Size * sizeof(FColor));
		Mip->BulkData.Unlock();

		// 다음 단계: 2x2 평균
		const int32 Half = Size / 2;
		TArray<FColor> Next;
		Next.SetNumUninitialized(Half * Half);
		for (int32 Y = 0; Y < Half; ++Y)
		{
			for (int32 X = 0; X < Half; ++X)
			{
				const FColor& A = Level[(Y * 2) * Size + X * 2];
				const FColor& B = Level[(Y * 2) * Size + X * 2 + 1];
				const FColor& C = Level[(Y * 2 + 1) * Size + X * 2];
				const FColor& D = Level[(Y * 2 + 1) * Size + X * 2 + 1];
				Next[Y * Half + X] = FColor(
					(A.R + B.R + C.R + D.R + 2) / 4,
					(A.G + B.G + C.G + D.G + 2) / 4,
					(A.B + B.B + C.B + D.B + 2) / 4,
					(A.A + B.A + C.A + D.A + 2) / 4);
			}
		}
		Level = MoveTemp(Next);
		Size = Half;
	}

	Texture->UpdateResource();

	MapTexture = Texture;
	MapCenter = Job->Center;
	MapPlayerZ = Job->PlayerZ;
	bMapIndoor = Job->bIndoor;
	bHasMap = true;

	LOG(TEXT("미니맵 지도 굽기 완료 (%dx%d, 실내=%s, 변환 %.0f ms)."), N, N, bMapIndoor ? TEXT("O") : TEXT("X"), Job->StylizeMs);
}
