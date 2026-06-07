#include "GM_Main.h"
#include "Py_Grey.h"

AGM_Main::AGM_Main()
{
	static ConstructorHelpers::FClassFinder<APy_Grey> PlayerClass(TEXT("Blueprint'/Game/UE_Asset/Character/Player/Grey/BP_PyGrey.BP_PyGrey_C'"));

	if (PlayerClass.Succeeded())
		DefaultPawnClass = PlayerClass.Class;
  
}
