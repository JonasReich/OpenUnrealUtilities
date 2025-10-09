// Copyright (c) 2023 Jonas Reich & Contributors

#include "Localization/OUUTextLibrary.h"

#include "HAL/PlatformFileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Internationalization/Culture.h"
#include "Internationalization/PolyglotTextData.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/StringTableRegistry.h"
#include "LogOpenUnrealUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/Csv/CsvParser.h"

#define LOCTEXT_NAMESPACE "OUUTextLibrary"

FText UOUUTextLibrary::FormatListText(const TArray<FText>& Texts, bool bFormatWithFinalAndSeparator)
{
	const int32 TextsNum = Texts.Num();
	switch (TextsNum)
	{
	case 0: return FText::GetEmpty();
	case 1: return Texts[0];
	default: break;
	}

	// Combine texts from back to start
	const int32 SecondToLastIdx = TextsNum - 2;
	FText CombinedText = bFormatWithFinalAndSeparator
		? FormatListText_FinalAndSeparator(Texts[SecondToLastIdx], Texts[SecondToLastIdx + 1])
		: FormatListText_GenericSeparator(Texts[SecondToLastIdx], Texts[SecondToLastIdx + 1]);

	for (int32 i = SecondToLastIdx - 1; i >= 0; i--)
	{
		CombinedText = FormatListText_GenericSeparator(Texts[i], CombinedText);
	}
	return CombinedText;
}

FText UOUUTextLibrary::FormatListText_GenericSeparator(const FText& TextA, const FText& TextB)
{
	return FText::FormatOrdered(LOCTEXT("List.CombineGenericItemsInList", "{0}, {1}"), TextA, TextB);
}

FText UOUUTextLibrary::FormatListText_FinalAndSeparator(const FText& TextA, const FText& TextB)
{
	return FText::FormatOrdered(LOCTEXT("List.CombineFinalItemsInList", "{0} and {1}"), TextA, TextB);
}

FText UOUUTextLibrary::JoinBy(const TArray<FText>& Texts, FText Separator)
{
	switch (Texts.Num())
	{
	case 0: return FText::GetEmpty();
	case 1: return Texts[0];
	default: break;
	}

	FText CombinedText = Texts[0];
	for (int32 i = 1; i < Texts.Num(); i++)
	{
		CombinedText = FText::FormatOrdered(INVTEXT("{0}{1}{2}"), CombinedText, Separator, Texts[i]);
	}
	return CombinedText;
}

void UOUUTextLibrary::RegisterPluginStringTable(
	const FString& InPluginName,
	const FName& InTableId,
	const FString& InNamespace,
	const FString& InPluginRelativeTablePath)
{
	auto pPlugin = IPluginManager::Get().FindPlugin(InPluginName);
	if (ensureMsgf(pPlugin, TEXT("Plugin %s not found"), *InPluginName))
	{
		FStringTableRegistry::Get()
			.Internal_LocTableFromFile(InTableId, InNamespace, InPluginRelativeTablePath, pPlugin->GetContentDir());
	}
}

void UOUUTextLibrary::RegisterPluginStringTable(const FString& InPluginName, const FString& InPluginRelativeTablePath)
{
	const FString FileBaseName = FPaths::GetBaseFilename(InPluginRelativeTablePath);
	RegisterPluginStringTable(InPluginName, *FileBaseName, FileBaseName, InPluginRelativeTablePath);
}

bool UOUUTextLibrary::ExportStringTableToCSV(const UObject* StringTable, const FString& ExportPath)
{
	if (const auto* CastedStringTable = Cast<UStringTable>(StringTable))
	{
		return CastedStringTable->GetStringTable()->ExportStrings(ExportPath);
	}
	return false;
}

void UOUUTextLibrary::LoadLocalizedTextsFromCSV(const FString& CsvDirectoryPath)
{
	auto PrioritizedCultureNames = FInternationalization::Get().GetCurrentCulture()->GetPrioritizedParentCultureNames();

	auto& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	TMap<FOUUTextIdentity, FPolyglotTextData> PolyglotData;
	PlatformFile.IterateDirectory(*CsvDirectoryPath, [&](const TCHAR* IteratePath, bool IsDirectory) -> bool {
		if (IsDirectory)
		{
			return true;
		}
		FString PathPart, NamePart, ExtensionPart;
		FPaths::Split(IteratePath, PathPart, NamePart, ExtensionPart);
		if (ExtensionPart.Equals(TEXT("csv"), ESearchCase::IgnoreCase) == false)
		{
			return true;
		}

		FString BaseNamePart, CulturePart;
		NamePart.Split(TEXT("_"), &BaseNamePart, &CulturePart);

		// Skip any culture codes that are not part of the active culture name list.
		// Allowing the entire list is required to e.g. allow having a base en translation with only a few en-US
		// overrides.
		if (PrioritizedCultureNames.Contains(CulturePart))
		{
			LoadLocalizedTextsFromCSV(IteratePath, CulturePart, PolyglotData);
		}
		return true;
	});

	TArray<FPolyglotTextData> PolyglotDataArray;
	for (auto& Entry : PolyglotData)
	{
		PolyglotDataArray.Add(Entry.Value);
	}

	// Register the polyglot data with the localization manager
	FTextLocalizationManager::Get().RegisterPolyglotTextData(PolyglotDataArray, true);
}

void UOUUTextLibrary::LoadLocalizedTextsFromCSV(
	const FString& CsvFilePath,
	const FString& Culture,
	TMap<FOUUTextIdentity, FPolyglotTextData>& InOutPolyglotTextData)
{
	FString CSVData;
	const bool bLoadedFile = FFileHelper::LoadFileToString(CSVData, *CsvFilePath);
	if (bLoadedFile == false)
	{
		UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("Failed to load csv file %s"), *CsvFilePath);
		return;
	}

	const FCsvParser CsvParser{CSVData};
	const FCsvParser::FRows& Rows = CsvParser.GetRows();
	if (Rows.Num() == 0)
	{
		UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("Failed to parse rows in csv file %s"), *CsvFilePath);
		return;
	}

	// Determine column indices
	auto& KeyRow = Rows[0];

	auto GetKeyIdx = [&KeyRow](const TCHAR* SearchKeyName) -> int32 {
		for (int32 HeaderIdx = 0; HeaderIdx < KeyRow.Num(); ++HeaderIdx)
		{
			const TCHAR* HeaderKey = KeyRow[HeaderIdx];
			if (FCString::Stricmp(HeaderKey, SearchKeyName) == 0)
			{
				return HeaderIdx;
			}
		}
		return INDEX_NONE;
	};

#define GET_KEY_IDX(KeyName)                                                                                           \
	int32 KeyName##Idx = INDEX_NONE;                                                                                   \
	{                                                                                                                  \
		KeyName##Idx = GetKeyIdx(TEXT(PREPROCESSOR_TO_STRING(KeyName)));                                               \
		if (KeyName##Idx == INDEX_NONE)                                                                                \
		{                                                                                                              \
			UE_LOG(                                                                                                    \
				LogOpenUnrealUtilities,                                                                                \
				Warning,                                                                                               \
				TEXT("Failed to load column %s of csv file %s"),                                                       \
				TEXT(PREPROCESSOR_TO_STRING(KeyName)),                                                                 \
				*CsvFilePath);                                                                                         \
			return;                                                                                                    \
		}                                                                                                              \
	}

	GET_KEY_IDX(Namespace)
	GET_KEY_IDX(Key)
	GET_KEY_IDX(SourceString)
	GET_KEY_IDX(LocalizedString)
#undef GET_KEY_IDX

	int32 NumLoctexts = 0;
	// Read all columns as polyglot data
	for (int32 RowIdx = 1; Rows.IsValidIndex(RowIdx); RowIdx++)
	{
		auto& ImportRow = Rows[RowIdx];
		auto& Namespace = ImportRow[NamespaceIdx];
		auto& Key = ImportRow[KeyIdx];
		FString SourceString = ImportRow[SourceStringIdx];
		SourceString = SourceString.ReplaceEscapedCharWithChar();
		if (SourceString.IsEmpty())
		{
			continue;
		}

		FString LocalizedString = ImportRow[LocalizedStringIdx];
		LocalizedString = LocalizedString.ReplaceEscapedCharWithChar();

		if (LocalizedString.IsEmpty())
		{
			continue;
		}

		auto* StringPtr = *LocalizedString;
		while (FChar::IsWhitespace(*StringPtr))
		{
			StringPtr++;
		}

		// Found non-whitespace non-terminator character in string...
		if (StringPtr && *StringPtr)
		{
			NumLoctexts++;

			ELocalizedTextSourceCategory TextSource = ELocalizedTextSourceCategory::Game;
#if WITH_EDITOR
			if (GIsEditor)
			{
				TextSource = ELocalizedTextSourceCategory::Editor;
			}
#endif
			auto& NewEntry = InOutPolyglotTextData.FindOrAdd(
				FOUUTextIdentity{Namespace, Key},
				FPolyglotTextData{TextSource, Namespace, Key, SourceString, TEXT("en")});
			NewEntry.AddLocalizedString(Culture, LocalizedString);
		}
	}

	UE_LOG(
		LogOpenUnrealUtilities,
		Log,
		TEXT("Imported %i loctexts for %s from %s"),
		NumLoctexts,
		*Culture,
		*CsvFilePath);
}

#undef LOCTEXT_NAMESPACE
