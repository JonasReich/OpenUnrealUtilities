// Copyright (c) 2023 Jonas Reich & Contributors

#include "Misc/MessageDialogLibrary.h"

#include "Misc/MessageDialog.h"

void UMessageDialogLibrary::ShowMessageDialogueNotification(FText OptionalTitle, FText Message)
{
	FMessageDialog::Debugf(Message, GetOptionalTitlePtr(OptionalTitle));
}

TEnumAsByte<EAppReturnType::Type> UMessageDialogLibrary::OpenMessageDialog(
	TEnumAsByte<EAppMsgType::Type> MessageType,
	FText OptionalTitle,
	FText Message)
{
	return FMessageDialog::Open(MessageType, Message, GetOptionalTitlePtr(OptionalTitle));
}

TEnumAsByte<EAppReturnType::Type> UMessageDialogLibrary::OpenMessageDialogWithDefaultValue(
	TEnumAsByte<EAppMsgType::Type> MessageType,
	TEnumAsByte<EAppReturnType::Type> DefaultValue,
	FText OptionalTitle,
	FText Message)
{
	return FMessageDialog::Open(
		StaticCast<EAppMsgType::Type>(MessageType),
		DefaultValue,
		Message,
		GetOptionalTitlePtr(OptionalTitle));
}

const FText* UMessageDialogLibrary::GetOptionalTitlePtr(FText& Text)
{
	const int32 TitleLength = Text.ToString().Len();
	return (TitleLength > 0) ? &Text : nullptr;
}
