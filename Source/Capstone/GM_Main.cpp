#include "GM_Main.h"
#include "Py_Grey.h"
#include "LevelTransitionSubsystem.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"

AGM_Main::AGM_Main()
{
	static ConstructorHelpers::FClassFinder<APy_Grey> PlayerClass(TEXT("Blueprint'/Game/UE_Asset/Character/Player/Grey/BP_PyGrey.BP_PyGrey_C'"));

	if (PlayerClass.Succeeded())
		DefaultPawnClass = PlayerClass.Class;

}

AActor* AGM_Main::ChoosePlayerStart_Implementation(AController* Player)
{
	FName WantedTag = NAME_None;
	if (ULevelTransitionSubsystem* TransitionSubsystem = GetGameInstance()->GetSubsystem<ULevelTransitionSubsystem>())
	{
		WantedTag = TransitionSubsystem->ConsumePendingPlayerStartTag();
	}

	// WantedTag가 None이면(=포탈을 거치지 않고 처음 시작한 경우) 태그가 없는 PlayerStart를 찾는다.
	// Super의 기본 동작은 태그와 무관하게 레벨의 모든 PlayerStart 중 랜덤으로 고르기 때문에,
	// 그대로 두면 문 앞처럼 태그가 붙은 PlayerStart까지 섞여서 랜덤 스폰되는 문제가 있었다.
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		if (It->PlayerStartTag == WantedTag)
			return *It;
	}

	// 태그가 일치하는 PlayerStart를 하나도 못 찾았을 때만 엔진 기본 동작으로 폴백
	return Super::ChoosePlayerStart_Implementation(Player);
}
