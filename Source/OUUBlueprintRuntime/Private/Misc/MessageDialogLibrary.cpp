// Copyright (c) 2023 Jonas Reich & Contributors

#include "Misc/MessageDialogLibrary.h"

#include "Misc/EngineVersionComparison.h"
#include "Misc/MessageDialog.h"

void UMessageDialogLibrary::ShowMessageDialogueNotification(FText OptionalTitle, FText Message)
{
	if (OptionalTitle.IsEmpty())
	{
		FMessageDialog::Debugf(Message);
	}
	else
	{
#if UE_VERSION_OLDER_THAN(5, 3, 0)
		FMessageDialog::Debugf(Message, &OptionalTitle);
#else
		FMessageDialog::Debugf(Message, OptionalTitle);
#endif
	}
}

TEnumAsByte<EAppReturnType::Type> UMessageDialogLibrary::OpenMessageDialog(
	TEnumAsByte<EAppMsgType::Type> MessageType,
	FText OptionalTitle,
	FText Message)
{
	if (OptionalTitle.IsEmpty())
	{
		return FMessageDialog::Open(MessageType, Message);
	}
#if UE_VERSION_OLDER_THAN(5, 3, 0)
	return FMessageDialog::Open(MessageType, Message, &OptionalTitle);
#else
	return FMessageDialog::Open(MessageType, Message, OptionalTitle);
#endif
}

TEnumAsByte<EAppReturnType::Type> UMessageDialogLibrary::OpenMessageDialogWithDefaultValue(
	TEnumAsByte<EAppMsgType::Type> MessageType,
	TEnumAsByte<EAppReturnType::Type> DefaultValue,
	FText OptionalTitle,
	FText Message)
{
	if (OptionalTitle.IsEmpty())
	{
		return FMessageDialog::Open(StaticCast<EAppMsgType::Type>(MessageType), DefaultValue, Message);
	}

#if UE_VERSION_OLDER_THAN(5, 3, 0)
	return FMessageDialog::Open(StaticCast<EAppMsgType::Type>(MessageType), DefaultValue, Message, &OptionalTitle);
#else
	return FMessageDialog::Open(StaticCast<EAppMsgType::Type>(MessageType), DefaultValue, Message, OptionalTitle);
#endif
}

//------------------------------------------------------------------------
// Console command
//------------------------------------------------------------------------

static FAutoConsoleCommand OpenActorMapCommand(
	TEXT("ouu.Modal.OpenSimpleModal"),
	TEXT("Open a simple modal window that displays a default text"),
	FConsoleCommandDelegate::CreateStatic([]() {
		TEnumAsByte<EAppMsgType::Type> MyType(EAppMsgType::Ok);
		UMessageDialogLibrary::OpenMessageDialog(
			MyType,
			FText::FromString("Default Title"),
			FText::FromString("Default Message"));
	}));
