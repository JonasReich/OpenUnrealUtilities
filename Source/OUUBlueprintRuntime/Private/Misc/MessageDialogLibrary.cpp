// Copyright (c) 2022 Jonas Reich

#include "Misc/MessageDialogLibrary.h"

#include "EditorDialogLibrary.h"
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
	return UEditorDialogLibrary::ShowMessage(OptionalTitle, Message, MessageType);
}

TEnumAsByte<EAppReturnType::Type> UMessageDialogLibrary::OpenMessageDialogWithDefaultValue(
	TEnumAsByte<EAppMsgType::Type> MessageType,
	TEnumAsByte<EAppReturnType::Type> DefaultValue,
	FText OptionalTitle,
	FText Message)
{
	return UEditorDialogLibrary::ShowMessage(OptionalTitle, Message, MessageType, DefaultValue);
}

const FText* UMessageDialogLibrary::GetOptionalTitlePtr(FText& Text)
{
	const int32 TitleLength = Text.ToString().Len();
	return (TitleLength > 0) ? &Text : nullptr;
}
