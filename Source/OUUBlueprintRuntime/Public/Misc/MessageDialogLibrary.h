// Copyright (c) 2021 Jonas Reich

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "MessageDialogLibrary.generated.h"

/** Blueprint exposed copy of EAppReturnType */
UENUM()
enum class EBlueprintAppReturnType : uint8
{
	No = 0,
	Yes = EAppReturnType::Yes,
	YesAll = EAppReturnType::YesAll,
	NoAll = EAppReturnType::NoAll,
	Cancel = EAppReturnType::Cancel,
	Ok = EAppReturnType::Ok,
	Retry = EAppReturnType::Retry,
	Continue = EAppReturnType::Continue,
};

static_assert(EBlueprintAppReturnType::No == static_cast<EBlueprintAppReturnType>(EAppReturnType::No),
	"EBlueprintAppReturnType::No must be equal to EAppReturnType::No");

/** Blueprint exposed copy of EAppMsgType */
UENUM()
enum class EBlueprintAppMsgType : uint8
{
	Ok = 0,
	YesNo = EAppMsgType::YesNo,
	OkCancel = EAppMsgType::OkCancel,
	YesNoCancel  = EAppMsgType::YesNoCancel,
	CancelRetryContinue = EAppMsgType::CancelRetryContinue,
	YesNoYesAllNoAll = EAppMsgType::YesNoYesAllNoAll,
	YesNoYesAllNoAllCancel = EAppMsgType::YesNoYesAllNoAllCancel,
	YesNoYesAll = EAppMsgType::YesNoYesAll
};

static_assert(EBlueprintAppMsgType::Ok == static_cast<EBlueprintAppMsgType>(EAppMsgType::Ok),
	"EBlueprintAppMsgType::Ok must be equal to EAppMsgType::Ok");

/**
* Library to open blocking message dialog popups from blueprint (editor utilities).
*/
UCLASS()
class OUUBLUEPRINTRUNTIME_API UMessageDialogLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 * Pops up a message dialog box containing the input string.
	 * @param OptionalTitle Optional title to use (defaults to "Message" if left empty)
	 * @param Message Text of message to show
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Messsage Dialog")
	static void ShowMessageDialogueNotification(FText OptionalTitle, FText Message);

	/**
	 * Open a modal message box dialog
	 * @param MessageType Controls buttons dialog should have
	 * @param OptionalTitle Optional title to use (defaults to "Message")
	 * @param Message Text of message to show
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Messsage Dialog")
	static EBlueprintAppReturnType OpenMessageDialog(EBlueprintAppMsgType MessageType, FText OptionalTitle, FText Message);

	/**
	 * Open a modal message box dialog
	 * @param MessageType Controls buttons dialog should have
	 * @param DefaultValue If the application is Unattended, the function will log and return DefaultValue
	 * @param OptionalTitle Optional title to use (defaults to "Message")
	 * @param Message Text of message to show
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Messsage Dialog")
	static EBlueprintAppReturnType OpenMessageDialogWithDefaultValue(EBlueprintAppMsgType MessageType, EBlueprintAppReturnType DefaultValue, FText OptionalTitle, FText Message);

private:
	static const FText* GetOptionalTitlePtr(FText& Text);
};
