// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Window/OUUWindowParameters.h"

#include "OUUWindow.generated.h"

class UWidget;
class UOUUWindow;
class SWindow;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOUUWindowClosed, UOUUWindow*, ClosedWindow);

/**
 * Blueprint wrapper for root window that can be initialized with an arbitrary UWidget root.
 * This allows you to create new editor/engine windows from Blueprint and add user widget trees to them.
 */
UCLASS(BlueprintType)
class UOUUWindow : public UObject
{
	GENERATED_BODY()
public:
	/**
	 * Called whenever the window is closed.
	 * This is called for all window close reasons:
	 * User closed window with X button, or window was closed via CloseWindow() function.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnOUUWindowClosed OnWindowClosed;

	/**
	 * Open a new OUUWindow.
	 * This should be the only way to create and initialize UOUUWindow objects.
	 */
	UFUNCTION(BlueprintCallable, Category = "UMG|Window")
	static UOUUWindow* OpenWindow(UWidget* InRootWidget, FOUUWindowParameters InWindowParameters, UObject* Outer);

	UFUNCTION(BlueprintPure, Category = "UMG|Window")
	bool IsOpened() const;

	/**
	 * Close the opened window.
	 * This does the same as closing the window via the X/close button.
	 * Does not do anything if the window was closed before, but emits a warning.
	 * You can call IsOpened() beforehand if you want to avoid this case.
	 * Windows cannot be reopened.
	 */
	UFUNCTION(BlueprintCallable, Category = "UMG|Window")
	void CloseWindow();

	/** @returns a copy of the cached parameters with which the window was initialized when opening (see OpenWindow) */
	UFUNCTION(BlueprintPure, Category = "UMG|Window")
	FORCEINLINE FOUUWindowParameters GetWindowParameters() const { return WindowParameters; }

protected:
	/** Was the window initialized via Intialize() function? */
	bool bIsInitialized = false;

	/** The slate window that was created for this window. */
	TSharedPtr<SWindow> SlateWindow = nullptr;

	/** Root widget of the slate window */
	UPROPERTY()
	UWidget* RootWidget = nullptr;

	/** Cached copy of parameters with which the window was initialized. */
	UPROPERTY()
	FOUUWindowParameters WindowParameters;

	/** Initialization. Should only be called from OpenWindow() */
	void Initialize(UWidget* InRootWidget, FOUUWindowParameters InWindowParameters);

	/** Function bound to the internal window closed event of the slate widget */
	void HandleSlateWindowClosed(const TSharedRef<SWindow>& ClosedSlateWindow);
};
