// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "OUUEditorLibrary.generated.h"

USTRUCT(BlueprintType)
struct FOUUBlueprintEditorFocusContent
{
	GENERATED_BODY()
public:
	/* Tab on which to focus (e.g. 'My Blueprint' tab). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString TabToFocusOrOpen;

	/* The GUID of a blueprint node */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString NodeGUID;

	/* Name of the outer object - should be the blueprint that 'owns' the node */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ObjectName;
};

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

	/** Open a bluerpint and focus on content inside if possible. */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Editor")
	static void FocusOnBlueprintContent(const FOUUBlueprintEditorFocusContent& FocusContent);

	/** Get the GUID for the currently selected blueprint node. Only works if a single node is selected in Editor. */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Editor")
	static FGuid GetCurrentlySelectedBlueprintNodeGuid();
};
