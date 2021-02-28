// Copyright (c) 2021 Jonas Reich

#include "Misc/MessageDialogLibrary.h"

#include "Misc/MessageDialog.h"

void UMessageDialogLibrary::ShowMessageDialogueNotification(FText OptionalTitle, FText Message)
{
	FMessageDialog::Debugf(Message, GetOptionalTitlePtr(OptionalTitle));
}

EBlueprintAppReturnType UMessageDialogLibrary::OpenMessageDialog(EBlueprintAppMsgType MessageType, FText OptionalTitle, FText Message)
{
	return StaticCast<EBlueprintAppReturnType>(FMessageDialog::Open(
		StaticCast<EAppMsgType::Type>(MessageType),
		Message,
		GetOptionalTitlePtr(OptionalTitle)));
}

EBlueprintAppReturnType UMessageDialogLibrary::OpenMessageDialogWithDefaultValue(EBlueprintAppMsgType MessageType,
	EBlueprintAppReturnType DefaultValue, FText OptionalTitle, FText Message)
{
	return StaticCast<EBlueprintAppReturnType>( FMessageDialog::Open(
		StaticCast<EAppMsgType::Type>(MessageType),
		StaticCast<EAppReturnType::Type>(DefaultValue),
		Message,
		GetOptionalTitlePtr(OptionalTitle)));
}

const FText* UMessageDialogLibrary::GetOptionalTitlePtr(FText& Text)
{
	const int32 TitleLength = Text.ToString().Len();
	return (TitleLength > 0) ? &Text : nullptr; 
}
