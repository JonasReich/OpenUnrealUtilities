// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "OUUEditorLibrary.generated.h"

UCLASS()
class OUUEDITOR_API UOUUEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Editor")
	static void InvokeSessionFrontend(FName Panel = "AutomationPanel");

	/**
	 * Reruns the construction scripts of an actor.
	 * Be careful when calling this, as it can have various unexpected side effects and is pretty slow.
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Editor")
	static void RerunConstructionScripts(AActor* Actor);
};
