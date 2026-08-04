// Copyright (c) 2023 Jonas Reich & Contributors

#include "WebBrowser/OUUWebBrowser.h"

#include "Async/TaskGraphInterfaces.h"
#include "LogOpenUnrealUtilities.h"
#include "SWebBrowser.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#if WITH_OUU_HERMES
	#include "HermesServer.h"
	#include "Modules/ModuleManager.h"
#endif

#define LOCTEXT_NAMESPACE "OUUWebBrowser"

UOUUWebBrowser::UOUUWebBrowser()
{
	bIsVariable = true;
}

void UOUUWebBrowser::LoadURL(FString NewURL)
{
	if (WebBrowserWidget.IsValid())
	{
		WebBrowserWidget->LoadURL(NewURL);
	}
}

void UOUUWebBrowser::LoadString(FString Contents, FString DummyURL)
{
	if (WebBrowserWidget.IsValid())
	{
		WebBrowserWidget->LoadString(Contents, DummyURL);
	}
}

void UOUUWebBrowser::ExecuteJavascript(const FString& ScriptText)
{
	if (WebBrowserWidget.IsValid())
	{
		WebBrowserWidget->ExecuteJavascript(ScriptText);
	}
}

FText UOUUWebBrowser::GetTitleText() const
{
	if (WebBrowserWidget.IsValid())
	{
		return WebBrowserWidget->GetTitleText();
	}

	return FText::GetEmpty();
}

FString UOUUWebBrowser::GetUrl() const
{
	if (WebBrowserWidget.IsValid())
	{
		return WebBrowserWidget->GetUrl();
	}

	return FString();
}

void UOUUWebBrowser::SynchronizeProperties()
{
	Super::SynchronizeProperties();
}

void UOUUWebBrowser::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	WebBrowserWidget.Reset();
}

#if WITH_EDITOR
const FText UOUUWebBrowser::GetPaletteCategory()
{
	return LOCTEXT("PaletteCategory", "Open Unreal Utilities");
}
#endif

TSharedRef<SWidget> UOUUWebBrowser::RebuildWidget()
{
	if (IsDesignTime())
	{
		// clang-format off
		return SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DesignTimeLabel", "OUU Web Browser"))
			];
		// clang-format on
	}

	// clang-format off
	WebBrowserWidget = SNew(SWebBrowser)
		.InitialURL(InitialURL)
		.ShowControls(false)
		.SupportsTransparency(bSupportsTransparency)
		.OnUrlChanged(BIND_UOBJECT_DELEGATE(FOnTextChanged, HandleOnUrlChanged))
		.OnBeforePopup(BIND_UOBJECT_DELEGATE(FOnBeforePopupDelegate, HandleOnBeforePopup))
		.OnConsoleMessage(BIND_UOBJECT_DELEGATE(FOnConsoleMessageDelegate, HandleOnConsoleMessage))
		.OnBeforeNavigation(BIND_UOBJECT_DELEGATE(SWebBrowser::FOnBeforeBrowse, HandleOnBeforeNavigation));
	// clang-format on

	return WebBrowserWidget.ToSharedRef();
}

void UOUUWebBrowser::HandleOnUrlChanged(const FText& Text)
{
	OnUrlChanged.Broadcast(Text);
}

void UOUUWebBrowser::HandleOnConsoleMessage(
	const FString& Message,
	const FString& Source,
	int32 Line,
	EWebBrowserConsoleLogSeverity Severity)
{
	OnConsoleMessage.Broadcast(Message, Source, Line);
}

bool UOUUWebBrowser::HandleOnBeforePopup(FString URL, FString Frame)
{
	if (OnBeforePopup.IsBound())
	{
		if (IsInGameThread())
		{
			OnBeforePopup.Broadcast(URL, Frame);
		}
		else
		{
			// Retry on the game thread, since the popup event can be raised from a background thread.
			TWeakObjectPtr<UOUUWebBrowser> WeakThis = this;
			FFunctionGraphTask::CreateAndDispatchWhenReady(
				[WeakThis, URL, Frame]() {
					if (WeakThis.IsValid())
					{
						WeakThis->HandleOnBeforePopup(URL, Frame);
					}
				},
				TStatId(),
				nullptr,
				ENamedThreads::GameThread);
		}

		return true;
	}

	return false;
}

bool UOUUWebBrowser::HandleOnBeforeNavigation(const FString& URL, const FWebNavigationRequest& Request)
{
#if WITH_OUU_HERMES
	// Chromium ignores the OS protocol handlers, so custom-scheme deep-links (e.g. tq2://content/...) would be
	// dropped. Hand them to the Hermes server for in-process dispatch instead and cancel the browser navigation.
	// This callback is invoked on the game thread, which is where Hermes dispatch must happen.
	if (IHermesServerModule* Hermes = FModuleManager::GetModulePtr<IHermesServerModule>("HermesServer"))
	{
		// HandleUrl returns false for any URL whose scheme is not the registered Hermes scheme, so http/https
		// and other regular navigations fall through and proceed normally.
		if (Hermes->HandleUrl(URL))
		{
			UE_LOG(LogOpenUnrealUtilities, Verbose, TEXT("Forwarded '%s' to the Hermes server"), *URL);
			return true;
		}
	}
#endif

	return false;
}

#undef LOCTEXT_NAMESPACE
