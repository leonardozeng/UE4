// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "ShowFlags.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DebugDrawService.generated.h"

class FSceneView;

/** 
 * 
 */
DECLARE_DELEGATE_TwoParams(FDebugDrawDelegate, class UCanvas*, class APlayerController*);

UCLASS(config=Engine)
class ENGINE_API UDebugDrawService : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	//void Register

	static FDelegateHandle Register(const TCHAR* Name, const FDebugDrawDelegate& NewDelegate);
	DELEGATE_DEPRECATED("This overload of Unregister is deprecated, instead pass the result of Register.")
	static void Unregister(const FDebugDrawDelegate& DelegateToRemove);
	static void Unregister(FDelegateHandle HandleToRemove);
	static void Draw(const FEngineShowFlags Flags, class UCanvas* Canvas);
	static void Draw(const FEngineShowFlags Flags, class FViewport* Viewport, FSceneView* View, FCanvas* Canvas);

private:
	static TArray<TArray<FDebugDrawDelegate> > Delegates;
	static FEngineShowFlags ObservedFlags;
};
