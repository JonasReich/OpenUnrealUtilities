// Copyright (c) 2021 Jonas Reich

#pragma once

#include "MessageLogName.h"
#include "MessageLogSeverity.h"
#include "MessageLogToken.h"

#include "MessageLogBlueprintLibrary.generated.h"

/**
 * Library for using message log in Blueprint.
 * Can be used in C++ as well of course, but it doesn't offer many advantages over FMessageLog itself.
 */
UCLASS()
class UMessageLogBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 * Get the FName of a message log name enum.
	 * Can be used to get the predefined engine message log names.
	 */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Message Log")
    static FName GetMessageLogName(EMessageLogName MessageLogName);

	/**
	 * Add a raw text message to the message log.
	 * @param MessageLogName: Name of the message log to add this message to. E.g. "PIE" for play-in-editor messages.
	 * @param MessageText: Complete text of the message
	 * @param Severity: The severity level of the message
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Message Log")
	static void AddTextMessageLogMessage(FName MessageLogName, FText MessageText, EMessageLogSeverity Severity = EMessageLogSeverity::Info);
	
	/**
	 * Add a tokenized message to the message log.
	 * Tokens allow composition of text messages containing interactive content other than just plain text.
	 * @param MessageLogName: Name of the message log to add this message to. E.g. "PIE" for play-in-editor messages.
	 * @param MessageTokens: All tokens the message consists of
	 * @param Severity: The severity level of the message
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Message Log")
	static void AddTokenizedMessageLogMessage(FName MessageLogName, TArray<FMessageLogToken> MessageTokens, EMessageLogSeverity Severity = EMessageLogSeverity::Info);

	/**
	 * Opens the message log window with the provided tab focused.
	 * @param InMinSeverity: Minimum severity of the messages being displayed. (e.g. Info displays all messages, Warning only Warnings and Errors)
	 * @param bOpenEvenIfEmpty: Force to open the message log even if it does not contain any messages
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Message Log")
	static void OpenMessageLog(FName MessageLogName, EMessageLogSeverity InMinSeverity = EMessageLogSeverity::Info, bool bOpenEvenIfEmpty = false );

	// -------------
	// FMessageLogToken constructor wrappers
	// -------------

	/**
	 * Create a message log token that references an asset.
	 * @param AssetName: Name/path of the asset to link to.
	 * @param OptionalLabelOverride: If not empty, this text is displayed as clickable link instead of the asset name itself
	 */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Message Log")
	static FMessageLogToken CreateAssetNameMessageLogToken(FString AssetName, FText OptionalLabelOverride);

	/**
	 * Creates a message log token that references any UObject. Can be clicked to navigate to actors in the scene.
	 * @param Object: Object to link to.
	 * @param OptionalLabelOverride: If not empty, this text is displayed as clickable link instead of the object name itself
	 */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Message Log")
	static FMessageLogToken CreateObjectMessageLogToken(UObject* Object, FText OptionalLabelOverride);

	/** Creates a message log token from a text */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Message Log")
    static FMessageLogToken CreateTextMessageLogToken(FText Text);

	/**
	 * Creates a message log token from a hyperlink URL. Can be clicked to open a browser to navigate to the linked webpage.
	 * @param URL: URL to link to
	 * @param OptionalLabelOverride: If not empty, this text is displayed as clickable link instead of the URL itself
	 */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Message Log")
    static FMessageLogToken CreateURLMessageLogToken(FString URL, FText OptionalLabelOverride);
};
