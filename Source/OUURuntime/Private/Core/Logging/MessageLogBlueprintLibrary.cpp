// Copyright (c) 2021 Jonas Reich

#include "Core/Logging/MessageLogBlueprintLibrary.h"

#include "Core/Logging/MessageLogToken.h"

FName UMessageLogBlueprintLibrary::GetMessageLogName(EMessageLogName MessageLogName)
{
#define MESSAGELOG_ENUM_TO_FNAME_CASE(Entry) case EMessageLogName::Entry: return PREPROCESSOR_TO_STRING(Entry);
	switch (MessageLogName)
	{
	MESSAGELOG_ENUM_TO_FNAME_CASE(PIE)
	MESSAGELOG_ENUM_TO_FNAME_CASE(EditorErrors)
	MESSAGELOG_ENUM_TO_FNAME_CASE(AssetTools)
	MESSAGELOG_ENUM_TO_FNAME_CASE(MapCheck)
	MESSAGELOG_ENUM_TO_FNAME_CASE(AssetCheck)
	MESSAGELOG_ENUM_TO_FNAME_CASE(AssetReimport)
	MESSAGELOG_ENUM_TO_FNAME_CASE(AnimBlueprintLog)
	MESSAGELOG_ENUM_TO_FNAME_CASE(AutomationTestingLog)
	MESSAGELOG_ENUM_TO_FNAME_CASE(BuildAndSubmitErrors)
	MESSAGELOG_ENUM_TO_FNAME_CASE(Blueprint)
	MESSAGELOG_ENUM_TO_FNAME_CASE(BlueprintLog)
	MESSAGELOG_ENUM_TO_FNAME_CASE(HLODResults)
	MESSAGELOG_ENUM_TO_FNAME_CASE(LightingResults)
	MESSAGELOG_ENUM_TO_FNAME_CASE(LoadErrors)
	MESSAGELOG_ENUM_TO_FNAME_CASE(LocalizationService)
	MESSAGELOG_ENUM_TO_FNAME_CASE(PackagingResults)
	MESSAGELOG_ENUM_TO_FNAME_CASE(SlateStyleLog)
	MESSAGELOG_ENUM_TO_FNAME_CASE(SourceControl)
	MESSAGELOG_ENUM_TO_FNAME_CASE(TranslationEditor)
	MESSAGELOG_ENUM_TO_FNAME_CASE(UDNParser)
	MESSAGELOG_ENUM_TO_FNAME_CASE(WidgetEvents)
		// return Blueprint log by default because it matches most situations
	default: return "Blueprint";
	}
#undef MESSAGELOG_ENUM_TO_FNAME_CASE
}

void UMessageLogBlueprintLibrary::AddTextMessageLogMessage(FName MessageLogName, FText MessageText, EMessageLogSeverity Severity)
{
	FMessageLog(MessageLogName).Message(StaticCast<EMessageSeverity::Type>(Severity), MessageText);
}

void UMessageLogBlueprintLibrary::AddTokenizedMessageLogMessage(FName MessageLogName, TArray<FMessageLogToken> MessageTokens,
                                                                EMessageLogSeverity Severity)
{
	if (MessageTokens.Num() == 0)
	{
		MessageTokens.Add(FMessageLogToken::CreateTextMessageLogToken(FText::FromString("<empty message>")));
	}
	TSharedRef<FTokenizedMessage> Message = FMessageLog(MessageLogName).Message(
		StaticCast<EMessageSeverity::Type>(Severity));
	for (auto& Token : MessageTokens)
	{
		Message->AddToken(Token.CreateNativeMessageToken());
	}
}

void UMessageLogBlueprintLibrary::OpenMessageLog(FName MessageLogName, EMessageLogSeverity InMinSeverity,
	bool bOpenEvenIfEmpty)
{
	FMessageLog(MessageLogName).Open(StaticCast<EMessageSeverity::Type>(InMinSeverity), bOpenEvenIfEmpty);
}

FMessageLogToken UMessageLogBlueprintLibrary::CreateAssetNameMessageLogToken(FString AssetName,
                                                                             FText OptionalLabelOverride)
{
	return FMessageLogToken::CreateAssetNameMessageLogToken(AssetName, OptionalLabelOverride);
}

FMessageLogToken UMessageLogBlueprintLibrary::CreateObjectMessageLogToken(
	UObject* Object, FText OptionalLabelOverride)
{
	return FMessageLogToken::CreateObjectMessageLogToken(Object, OptionalLabelOverride);
}

FMessageLogToken UMessageLogBlueprintLibrary::CreateTextMessageLogToken(FText Text)
{
	return FMessageLogToken::CreateTextMessageLogToken(Text);
}

FMessageLogToken UMessageLogBlueprintLibrary::CreateURLMessageLogToken(FString URL,
                                                                       FText OptionalLabelOverride)
{
	return FMessageLogToken::CreateURLMessageLogToken(URL, OptionalLabelOverride);
}
