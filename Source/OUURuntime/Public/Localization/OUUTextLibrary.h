// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "OUUTextLibrary.generated.h"

/**
 * Library for various math utilities not included in FMath or U
 */
UCLASS()
class OUURUNTIME_API UOUUTextLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Format a text that combines a variable number of items into a list text.
	 * e.g. from an input of n texts, a single text is created:
	 * - ["foo"] -> "foo"
	 * - ["foo", "bar"] -> "foo and bar"
	 * - ["foo", "bar", "foobar"] -> "foo, bar and foobar"
	 * Supports an arbitrary number of items.
	 *
	 * @param	Texts							List of texts components to combine into a list.
	 * @param	bFormatWithFinalAndSeparator	If true, the last two items in the list are combined with a special word
	 *											("and" if untranslated). If false, the final two items also use the
	 *											generic separator ("," if untranslated).
	 */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Text")
	static FText FormatListText(const TArray<FText>& Texts, bool bFormatWithFinalAndSeparator);

	/** Combined two texts with a generic separator ("," if untranslated). */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Text")
	static FText FormatListText_GenericSeparator(const FText& TextA, const FText& TextB);

	/** Combined two texts with a "final" separator ("and" if untranslated). */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Text")
	static FText FormatListText_FinalAndSeparator(const FText& TextA, const FText& TextB);

	/** Combine an arbitrary number of texts with a given separator */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Text")
	static FText JoinBy(const TArray<FText>& Texts, FText Separator);

	// Register a CSV string table for a plugin by resolving the file from plugin install directory.
	// The string table is NOT automatically collected by the localization gatherer in-full when using this (which it
	// would be when using the LOCTABLE_FROM_FILE_X macros), so you will need some other process to include the CSV
	// source strings!
	// @param InPluginRelativeTablePath path to csv (including extension) relative to the plugin's content directory
	static void RegisterPluginStringTable(
		const FString& InPluginName,
		const FName& InTableId,
		const FString& InNamespace,
		const FString& InPluginRelativeTablePath);

	// Register a CSV string table for a plugin (see above).
	// This overload registers the string table with table and namespace derived from the base name of the input file.
	// @param InPluginRelativeTablePath path to csv (including extension) relative to the plugin's content directory
	static void RegisterPluginStringTable(const FString& InPluginName, const FString& InPluginRelativeTablePath);

	/**
	 * Export the key, string, and meta-data information in this string table to a CSV file (does not export the
	 * namespace).
	 * Because UStringTable is not blueprint exposed, we need to pass it as UObject.
	 * @param ExportPath Disk file path of the target csv file, including extension
	 * @returns if the export succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Text")
	static bool ExportStringTableToCSV(const UObject* StringTable, const FString& ExportPath);

	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Text")
	static TSet<FString> GetCSVTranslationCultureNames(const FString& CsvDirectoryPath);

	// Load all CSV files from a given folder as polyglot data.
	// The culture is assumed to be added as a suffix to the file names, i.e. filename_culture.csv,
	// e.g. translations_de.csv, or translations_en-US.csv
	// The culture codes must be registered for the project for this to load correctly into translation memory.
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Text")
	static void LoadLocalizedTextsFromCSV(const FString& CsvDirectoryPath);

private:
	struct FOUUTextIdentity
	{
	public:
		FOUUTextIdentity(FString Namespace, FString Key) : Namespace(MoveTemp(Namespace)), Key(MoveTemp(Key)) {}

		FString Namespace;
		FString Key;

		FORCEINLINE bool operator==(const FOUUTextIdentity& Other) const
		{
			return Namespace == Other.Namespace && Key == Other.Key;
		}

		FORCEINLINE bool operator!=(const FOUUTextIdentity& Other) const { return !(Other == *this); }

		friend inline uint32 GetTypeHash(const FOUUTextIdentity& Id)
		{
			return HashCombine(GetTypeHash(Id.Namespace), GetTypeHash(Id.Key));
		}
	};

	static void LoadLocalizedTextsFromCSV(
		const FString& CsvFilePath,
		const FString& Culture,
		TMap<FOUUTextIdentity, FPolyglotTextData>& InOutPolyglotTextData);
};
