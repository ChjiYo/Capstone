#pragma once

#include "EngineMinimal.h"
#include "EngineGlobals.h"
#include "DestructibleComponent.h"
#include "Engine.h"
#include "Engine/AssetManager.h"
#include "Runtime/Engine/Classes/Engine/GameEngine.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NavigationPath.h"

UENUM(BlueprintType)
enum class EDir : uint8
{
	Front,
	Back,
	Right,
	Left
};

DECLARE_LOG_CATEGORY_EXTERN(UE7, Log, All);
#define LOG_CALLINFO		(FString(__FUNCTION__) + TEXT("{ ") + FString::FromInt(__LINE__) + TEXT(" }")) 
#define LOG(Format, ...)		UE_LOG(UE7, Warning, TEXT("%s : %s"), *LOG_CALLINFO, *FString::Printf(Format, ##__VA_ARGS__))


inline void PrintViewport(float fTime, const FColor& Color, const FString& strText)
{
	GEngine->AddOnScreenDebugMessage(-1, fTime, Color, strText);
}