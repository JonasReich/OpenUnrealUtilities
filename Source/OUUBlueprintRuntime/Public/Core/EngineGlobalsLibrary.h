// Copyright (c) 2021 Jonas Reich

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "EngineGlobalsLibrary.generated.h"

/**
* Engine globals defined in CoreGlobals.h and Build.h that are not blueprint exposed via engine libraries.
* Does not expand upon existing C++ functionality but merely makes it available for blueprint use. 
*/
UCLASS()
class UEngineGlobalsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** If this build was compiled with editor configuration */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Engine Globals|Build Configuration")
	static bool IsEditorBuild();
	
	/** If this build was compiled with debug configuration */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Engine Globals|Build Configuration")
	static bool IsDebugBuild();
	
	/** If this build was compiled with development configuration */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Engine Globals|Build Configuration")
	static bool IsDevelopmentBuild();
	
	/** If this build was compiled with test configuration */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Engine Globals|Build Configuration")
	static bool IsTestBuild();
	
	/** If this build was compiled with shipping configuration */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Engine Globals|Build Configuration")
	static bool IsShippingBuild();
	
	/** Check to see if this executable is running a commandlet (custom command-line processing code in an editor-like environment) */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Engine Globals|Runtime Mode")
	static bool IsRunningCommandlet();

	/** Check to see if we should initialise RHI and set up scene for rendering even when running a commandlet. */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Engine Globals|Runtime Mode")
	static bool AllowCommandletRendering();

	/** Check to see if we should initialize audio devices, etc for playing back sounds even when running a commandlet. */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Engine Globals|Runtime Mode")
	static bool AllowCommandletAudio();

	/**
	 * True if we are in the editor.
	 * Note that this is still true when using Play In Editor.
	 * You may want to use GWorld->HasBegunPlay in that case.
	 */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Engine Globals|Runtime Mode")
	static bool IsEditor();

	/**
	 * Whether engine was launched as a client. Not exclusive opposite to IsServer()!
	 * E.g. in Editor both IsClient() and IsServer() will return true.
	 */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Engine Globals|Runtime Mode")
	static bool IsClient();

	/**
	* Whether engine was launched as a server. Not exclusive opposite to IsClient()!
	* E.g. in Editor both IsClient() and IsServer() will return true.
	*/
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Engine Globals|Runtime Mode")
	static bool IsServer();

	/** This specifies whether the engine was launched as a build machine process.*/
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Engine Globals|Runtime Mode")
	static bool IsBuildMachine();

	/** If the current execution scope is in the game thread. */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Engine Globals|Runtime Mode")
	static bool IsInGameThread();
};
