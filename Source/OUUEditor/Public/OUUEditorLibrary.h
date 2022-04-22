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
};
