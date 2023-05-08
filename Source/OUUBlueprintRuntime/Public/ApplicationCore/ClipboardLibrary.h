// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "ClipboardLibrary.generated.h"

/** Exposes OS clipboard to Blueprint */
UCLASS()
class UClipboardLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Copies text to the operating system clipboard. */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Application Core|Clipboard")
	static void ClipboardCopy(const FString& SourceString);

	/** @returns text from the operating system clipboard. */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Application Core|Clipboard")
	static FString ClipboardPaste();
};
