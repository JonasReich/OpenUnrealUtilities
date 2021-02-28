// Copyright (c) 2021 Jonas Reich

#include "Slate/SlateNotificationLibrary.h"

#include "LogOpenUnrealUtilities.h"
#include "Components/Widget.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

FNotificationInfo FSlateNotificationInfo::ToNativeNotificationInfo() const
{
	FNotificationInfo Result(Text);
	if (bShowImage)
	{
		Result.Image = new FSlateBrush(Image);
	}
	Result.bUseThrobber = bUseThrobber;
	Result.bUseSuccessFailIcons = bUseSuccessFailIcons;
	Result.bUseLargeFont = bUseLargeFont;
	if (WidthOverride > 0.f)
	{
		Result.WidthOverride = WidthOverride; 
	}
	Result.bFireAndForget = bFireAndForget;
	return Result;
}

void FSlateNotificationHandle::NewGuid()
{
	FPlatformMisc::CreateGuid(NotificationId);
}

bool FSlateNotificationHandle::IsValid() const
{
	return NotificationId.IsValid();
}

bool FSlateNotificationHandle::operator==(const FSlateNotificationHandle& Other) const
{
	return Other.NotificationId == NotificationId;
}

FSlateNotificationHandle FSlateNotificationHandle::InvalidHandle()
{
	return FSlateNotificationHandle();
}

uint32 GetTypeHash(const FSlateNotificationHandle& Handle)
{
	return GetTypeHash(Handle.NotificationId);
}

ESlateNotificationState SlateNotificationState_ConvertSlateToBlueprint(SNotificationItem::ECompletionState State)
{
	switch(State)
	{
	case SNotificationItem::CS_Pending: return ESlateNotificationState::Pending;
	case SNotificationItem::CS_Success: return ESlateNotificationState::Success;
	case SNotificationItem::CS_Fail: return ESlateNotificationState::Fail;
	default: return ESlateNotificationState::None;
	}
}

SNotificationItem::ECompletionState SlateNotificationState_ConvertBlueprintToSlate(ESlateNotificationState State)
{
	switch(State)
	{
	case ESlateNotificationState::Pending: return SNotificationItem::CS_Pending;
	case ESlateNotificationState::Success: return SNotificationItem::CS_Success;
	case ESlateNotificationState::Fail: return SNotificationItem::CS_Fail;
	default: return SNotificationItem::CS_None;
	}
}

FSlateNotificationHandle USlateNotificationLibrary::AddSlateNotification(const FSlateNotificationInfo& Info)
{
	FSlateNotificationHandle Handle;
	while(!Handle.IsValid() || GetNotificationMap().Contains(Handle))
	{
		Handle.NewGuid();
	}
	if(!IsInGameThread())
	{
		UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("Failed to create slate notification, "
			"because AddSlateNotification was called from a thread that is not the game thread."));
		return FSlateNotificationHandle::InvalidHandle();
	}
	const auto NewNotification = FSlateNotificationManager::Get().AddNotification(Info.ToNativeNotificationInfo());
	GetNotificationMap().Add(Handle, NewNotification);
	return Handle;
}

void USlateNotificationLibrary::SetSlateNotificationText(const FSlateNotificationHandle& NotificationHandle, FText Text)
{
	if (auto* Notification = FindNotification(NotificationHandle))
	{
		Notification->SetText(Text);
	}
}

void USlateNotificationLibrary::SetSlateNotificationHyperlink(const FSlateNotificationHandle& NotificationHandle, FText HyperlinkText, FSlateNotificationSimpleSingleCastDelegate HyperlinkDelegate)
{
	if (auto* Notification = FindNotification(NotificationHandle))
	{
		const FSimpleDelegate Delegate = FSimpleDelegate::CreateLambda([=]()
		{
			HyperlinkDelegate.ExecuteIfBound();	
		});
		Notification->SetHyperlink(Delegate, HyperlinkText);
	}
}

void USlateNotificationLibrary::SetSlateNotificationExpireDuration(const FSlateNotificationHandle& NotificationHandle, float ExpireDuration)
{
	if (auto* Notification = FindNotification(NotificationHandle))
	{
		Notification->SetExpireDuration(ExpireDuration);
	}
}

void USlateNotificationLibrary::SetSlateNotificationFadeInDuration(const FSlateNotificationHandle& NotificationHandle, float FadeInDuration)
{
	if (auto* Notification = FindNotification(NotificationHandle))
	{
		Notification->SetFadeInDuration(FadeInDuration);
	}
}

void USlateNotificationLibrary::SetSlateNotificationFadeOutDuration(const FSlateNotificationHandle& NotificationHandle, float FadeOutDuration)
{
	if (auto* Notification = FindNotification(NotificationHandle))
	{
		Notification->SetFadeOutDuration(FadeOutDuration);
	}
}

ESlateNotificationState USlateNotificationLibrary::GetSlateNotificationCompletionState(const FSlateNotificationHandle& NotificationHandle)
{
	if (auto* Notification = FindNotification(NotificationHandle))
	{
		return SlateNotificationState_ConvertSlateToBlueprint(Notification->GetCompletionState());
	}
	return ESlateNotificationState::None;
}

void USlateNotificationLibrary::SetSlateNotificationCompletionState(const FSlateNotificationHandle& NotificationHandle, ESlateNotificationState CompletionState)
{
	if (auto* Notification = FindNotification(NotificationHandle))
	{
		Notification->SetCompletionState(SlateNotificationState_ConvertBlueprintToSlate(CompletionState));
	}
}

void USlateNotificationLibrary::ExpireSlateNotificationAndFadeout(const FSlateNotificationHandle& NotificationHandle)
{
	if (auto* Notification = FindNotification(NotificationHandle))
	{
		Notification->ExpireAndFadeout();
	}
}

void USlateNotificationLibrary::FadeoutSlateNotification(const FSlateNotificationHandle& NotificationHandle)
{
	if (auto* Notification = FindNotification(NotificationHandle))
	{
		Notification->Fadeout();
	}
}

void USlateNotificationLibrary::PulseSlateNotification(const FSlateNotificationHandle& NotificationHandle, FLinearColor GlowColor)
{
	if (auto* Notification = FindNotification(NotificationHandle))
	{
		Notification->Pulse(GlowColor);
	}
}

void USlateNotificationLibrary::ReleaseSlateNotificationHandle(const FSlateNotificationHandle& NotificationHandle)
{
	GetNotificationMap().Remove(NotificationHandle);
}

USlateNotificationLibrary::FNotificationMap& USlateNotificationLibrary::GetNotificationMap()
{
	return GetMutableDefault<USlateNotificationLibrary>()->RegisteredNotifications;
}

SNotificationItem* USlateNotificationLibrary::FindNotification(const FSlateNotificationHandle& Handle)
{
	UE_CLOG(!Handle.IsValid(), LogOpenUnrealUtilities, Warning, TEXT("Slate notification handle is invalid"));
	if (auto* Ptr = GetNotificationMap().Find(Handle))
	{
		return Ptr->Get();
	}
	UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("Failed to find notification."));
	return nullptr;
}
