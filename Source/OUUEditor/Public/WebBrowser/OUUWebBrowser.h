// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Components/Widget.h"

#include "OUUWebBrowser.generated.h"

class SWebBrowser;
enum class EWebBrowserConsoleLogSeverity;
struct FWebNavigationRequest;

/**
 * Editor web browser widget wrapping the Slate SWebBrowser.
 *
 * It exposes the same authoring surface as the engine UWebBrowser, but additionally intercepts navigations to
 * Hermes custom-scheme URLs (e.g. tq2://content/Game/...) and forwards them to the Hermes server in-process
 * instead of letting the underlying Chromium (CEF) instance try - and fail - to resolve them. Chromium ignores
 * the OS protocol handlers, so without this interception such deep-links embedded in loaded HTML would be
 * silently dropped. Forwarding is only compiled in on Win64 (where Hermes is available).
 *
 * This is an editor-only widget: both the web browser and Hermes stacks are editor/Win64 scoped.
 */
UCLASS()
class OUUEDITOR_API UOUUWebBrowser : public UWidget
{
	GENERATED_BODY()
public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOUUWebBrowserUrlChanged, const FText&, Text);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOUUWebBrowserBeforePopup, FString, URL, FString, Frame);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
		FOnOUUWebBrowserConsoleMessage,
		const FString&,
		Message,
		const FString&,
		Source,
		int32,
		Line);

	UOUUWebBrowser();

	/**
	 * Load the specified URL.
	 * @param NewURL New URL to load.
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Web Browser")
	void LoadURL(FString NewURL);

	/**
	 * Load a string as data to create a web page.
	 * @param Contents String to load.
	 * @param DummyURL Dummy URL for the page.
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Web Browser")
	void LoadString(FString Contents, FString DummyURL);

	/**
	 * Execute a JavaScript string in the context of the web page.
	 * @param ScriptText JavaScript string to execute.
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Web Browser")
	void ExecuteJavascript(const FString& ScriptText);

	/** @returns the current title of the web page. */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Web Browser")
	FText GetTitleText() const;

	/** @returns the currently loaded URL, or an empty string if no document is loaded. */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Web Browser")
	FString GetUrl() const;

	/** Called when the URL changes. */
	UPROPERTY(BlueprintAssignable, Category = "Open Unreal Utilities|Web Browser|Event")
	FOnOUUWebBrowserUrlChanged OnUrlChanged;

	/** Called when a popup is about to spawn. */
	UPROPERTY(BlueprintAssignable, Category = "Open Unreal Utilities|Web Browser|Event")
	FOnOUUWebBrowserBeforePopup OnBeforePopup;

	/** Called when the browser has console output to print. */
	UPROPERTY(BlueprintAssignable, Category = "Open Unreal Utilities|Web Browser|Event")
	FOnOUUWebBrowserConsoleMessage OnConsoleMessage;

	// -- UWidget
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	/** URL that the browser will initially navigate to. The URL should include the protocol, e.g. http:// */
	UPROPERTY(EditAnywhere, Category = Appearance)
	FString InitialURL;

	/** Whether the browser window should support transparency. */
	UPROPERTY(EditAnywhere, Category = Appearance)
	bool bSupportsTransparency = false;

	/** The underlying Slate web browser widget. */
	TSharedPtr<SWebBrowser> WebBrowserWidget;

	// -- UWidget
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Bound to SWebBrowser::OnUrlChanged; rebroadcasts via OnUrlChanged. */
	void HandleOnUrlChanged(const FText& Text);

	/** Bound to SWebBrowser::OnConsoleMessage; rebroadcasts via OnConsoleMessage (severity is dropped). */
	void HandleOnConsoleMessage(
		const FString& Message,
		const FString& Source,
		int32 Line,
		EWebBrowserConsoleLogSeverity Severity);

	/** Bound to SWebBrowser::OnBeforePopup; rebroadcasts via OnBeforePopup on the game thread. */
	bool HandleOnBeforePopup(FString URL, FString Frame);

	/**
	 * Bound to SWebBrowser::OnBeforeNavigation. Forwards Hermes custom-scheme URLs to the Hermes server
	 * in-process and cancels the browser navigation; all other navigations proceed normally.
	 * @returns true if the navigation was handled (and should be cancelled), false to let it proceed.
	 */
	bool HandleOnBeforeNavigation(const FString& URL, const FWebNavigationRequest& Request);
};
