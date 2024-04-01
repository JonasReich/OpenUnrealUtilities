// Copyright (c) 2023 Jonas Reich & Contributors

#include "Misc/MessageDialogLibrary.h"

#include "Misc/MessageDialog.h"

void UMessageDialogLibrary::ShowMessageDialogueNotification(FText OptionalTitle, FText Message)
{
	if (OptionalTitle.IsEmpty())
	{
		FMessageDialog::Debugf(Message, OptionalTitle);
	}
	else
	{
		FMessageDialog::Debugf(Message);
	}
}

TEnumAsByte<EAppReturnType::Type> UMessageDialogLibrary::OpenMessageDialog(
	TEnumAsByte<EAppMsgType::Type> MessageType,
	FText OptionalTitle,
	FText Message)
{
	if (OptionalTitle.IsEmpty())
	{
		return FMessageDialog::Open(MessageType, Message, OptionalTitle);
	}
	return FMessageDialog::Open(MessageType, Message);
}

TEnumAsByte<EAppReturnType::Type> UMessageDialogLibrary::OpenMessageDialogWithDefaultValue(
	TEnumAsByte<EAppMsgType::Type> MessageType,
	TEnumAsByte<EAppReturnType::Type> DefaultValue,
	FText OptionalTitle,
	FText Message)
{
	if (OptionalTitle.IsEmpty())
	{
		return FMessageDialog::Open(StaticCast<EAppMsgType::Type>(MessageType), DefaultValue, Message, OptionalTitle);
	}

	return FMessageDialog::Open(StaticCast<EAppMsgType::Type>(MessageType), DefaultValue, Message);
}
