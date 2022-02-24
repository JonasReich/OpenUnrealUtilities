// Copyright (c) 2022 Jonas Reich

#include "Window/OUUWindow.h"

#include "Components/Widget.h"
#include "Framework/Application/SlateApplication.h"
#include "LogOpenUnrealUtilities.h"
#include "Widgets/SWindow.h"

UOUUWindow* UOUUWindow::OpenWindow(UWidget* InRootWidget, FOUUWindowParameters InWindowParameters, UObject* Outer)
{
	UOUUWindow* NewWindowObject = NewObject<UOUUWindow>(Outer);
	NewWindowObject->Initialize(InRootWidget, InWindowParameters);
	return NewWindowObject;
}

bool UOUUWindow::IsOpened() const
{
	UE_CLOG(
		!bIsInitialized,
		LogOpenUnrealUtilities,
		Error,
		TEXT("IsOpened() was called on OUUWindow %s that was not initialized"),
		*GetName());

	return SlateWindow.IsValid();
}

void UOUUWindow::CloseWindow()
{
	UE_CLOG(
		!bIsInitialized,
		LogOpenUnrealUtilities,
		Error,
		TEXT("CloseWindow() was called on OUUWindow %s that was not initialized"),
		*GetName());

	if (IsOpened())
	{
		SlateWindow->RequestDestroyWindow();
	}
	else
	{
		UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("CloseWindow() was called on a window that was already closed"))
	}
}

void UOUUWindow::Initialize(UWidget* InRootWidget, FOUUWindowParameters InWindowParameters)
{
	UE_CLOG(
		bIsInitialized,
		LogOpenUnrealUtilities,
		Error,
		TEXT("Initialize() was called on OUUWindow %s that was already initialized"),
		*GetName());

	if (!ensure(IsValid(InRootWidget)))
	{
		UE_LOG(
			LogOpenUnrealUtilities,
			Error,
			TEXT("Could not inialize new OUUWindow %s with invalid Root Widget!"),
			*GetName());
		return;
	}

	if (!ensure(!SlateWindow.IsValid()))
	{
		UE_LOG(
			LogOpenUnrealUtilities,
			Error,
			TEXT("Initialize() was called on a UOUUWindow %s that already has a valid slate widget"),
			*GetName());
		return;
	}

	// Assign members from parameters
	RootWidget = InRootWidget;
	WindowParameters = InWindowParameters;

	// clang-format off
	SlateWindow = SNew(SWindow)
		.AutoCenter(StaticCast<EAutoCenter>((WindowParameters.AutoCenterRule)))
		.IsInitiallyMaximized(WindowParameters.bIsInitiallyMaximized)
		.ScreenPosition(WindowParameters.ScreenPosition)
		.CreateTitleBar(WindowParameters.bCreateTitleBar)
		.SizingRule(StaticCast<ESizingRule>(WindowParameters.SizingRule))
		.SupportsMaximize(WindowParameters.bSupportsMaximize)
		.SupportsMinimize(WindowParameters.bSupportsMinimize)
		.HasCloseButton(WindowParameters.bHasCloseButton)
		.Style(&FCoreStyle::Get().GetWidgetStyle<FWindowStyle>("Window"))
		.ClientSize(WindowParameters.ClientSize)
		.UseOSWindowBorder(WindowParameters.bUseOSWindowBorder)
		.Title(WindowParameters.Title);
	// clang-format on

	SlateWindow->SetOnWindowClosed(FOnWindowClosed::CreateUObject(this, &UOUUWindow::HandleSlateWindowClosed));

	SlateWindow->SetContent(RootWidget->TakeWidget());
	FSlateApplication::Get().AddWindow(SlateWindow.ToSharedRef());
	FSlateApplication::Get().GetRenderer()->CreateViewport(SlateWindow.ToSharedRef());

	bIsInitialized = true;
}

void UOUUWindow::HandleSlateWindowClosed(const TSharedRef<SWindow>& ClosedSlateWindow)
{
	ensure(ClosedSlateWindow == SlateWindow.ToSharedRef());
	SlateWindow.Reset();
	OnWindowClosed.Broadcast(this);
}
