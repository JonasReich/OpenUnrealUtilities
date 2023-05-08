// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "MessageDialogLibrary.generated.h"

/**
 * Library to open blocking message dialog popups from blueprint.
 * In contrast to UEditorDialogLibrary, these also work in non-editor builds in case a message log is present.
 */
UCLASS()
class OUUBLUEPRINTRUNTIME_API UMessageDialogLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 * Pops up a message dialog box containing the input string.
	 * @param	OptionalTitle	Optional title to use (defaults to "Message" if left empty)
	 * @param	Message			Text of message to show
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Messsage Dialog")
	static void ShowMessageDialogueNotification(FText OptionalTitle, FText Message);

	/**
	 * Open a modal message box dialog
	 * @param	MessageType		Controls buttons dialog should have
	 * @param	OptionalTitle	Optional title to use (defaults to "Message")
	 * @param	Message			Text of message to show
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Messsage Dialog")
	static TEnumAsByte<EAppReturnType::Type> OpenMessageDialog(
		TEnumAsByte<EAppMsgType::Type> MessageType,
		FText OptionalTitle,
		FText Message);

	/**
	 * Open a modal message box dialog
	 * @param	MessageType		Controls buttons dialog should have
	 * @param	DefaultValue	If the application is Unattended, the function will log and return DefaultValue
	 * @param	OptionalTitle	Optional title to use (defaults to "Message")
	 * @param	Message			Text of message to show
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Messsage Dialog")
	static TEnumAsByte<EAppReturnType::Type> OpenMessageDialogWithDefaultValue(
		TEnumAsByte<EAppMsgType::Type> MessageType,
		TEnumAsByte<EAppReturnType::Type> DefaultValue,
		FText OptionalTitle,
		FText Message);

private:
	static const FText* GetOptionalTitlePtr(FText& Text);
};
