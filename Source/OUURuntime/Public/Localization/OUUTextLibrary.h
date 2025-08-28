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

	/**
	 * Export the key, string, and meta-data information in this string table to a CSV file (does not export the
	 * namespace)
	 * @returns if the export succeeded
	 */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Text")
	static bool ExportStringTableToCSV(const UStringTable* StringTable, const FString& ExportPath);

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
