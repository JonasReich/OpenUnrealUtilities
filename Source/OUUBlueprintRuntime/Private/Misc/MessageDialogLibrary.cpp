// Copyright (c) 2023 Jonas Reich & Contributors

#include "Misc/MessageDialogLibrary.h"

#include "HAL/IConsoleManager.h"
#include "Misc/MessageDialog.h"

void UMessageDialogLibrary::ShowMessageDialogueNotification(FText OptionalTitle, FText Message)
{
	if (OptionalTitle.IsEmpty())
	{
		FMessageDialog::Debugf(Message);
	}
	else
	{
		FMessageDialog::Debugf(Message, OptionalTitle);
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

	return FMessageDialog::Open(MessageType, Message, OptionalTitle);
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

	return FMessageDialog::Open(StaticCast<EAppMsgType::Type>(MessageType), DefaultValue, Message, OptionalTitle);
}
