// Copyright (c) 2022 Jonas Reich

#include "Logging/MessageLogBlueprintLibrary.h"

#include "Logging/MessageLog.h"
#include "Logging/MessageLogToken.h"

FName UMessageLogBlueprintLibrary::GetMessageLogName(EMessageLogName MessageLogName)
{
	return ::GetMessageLogName(MessageLogName);
}

void UMessageLogBlueprintLibrary::AddTextMessageLogMessage(
	FName MessageLogName,
	FText MessageText,
	EMessageLogSeverity Severity)
{
	FMessageLog(MessageLogName).Message(StaticCast<EMessageSeverity::Type>(Severity), MessageText);
}

void UMessageLogBlueprintLibrary::AddTokenizedMessageLogMessage(
	FName MessageLogName,
	TArray<FMessageLogToken> MessageTokens,
	EMessageLogSeverity Severity)
{
	if (MessageTokens.Num() == 0)
	{
		MessageTokens.Add(FMessageLogToken::CreateTextMessageLogToken(FText::FromString("<empty message>")));
	}

	TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(StaticCast<EMessageSeverity::Type>(Severity));
	for (auto& Token : MessageTokens)
	{
		Message->AddToken(Token.CreateNativeMessageToken());
	}
	FMessageLog(MessageLogName).AddMessage(Message);
}

void UMessageLogBlueprintLibrary::OpenMessageLog(
	FName MessageLogName,
	EMessageLogSeverity InMinSeverity,
	bool bOpenEvenIfEmpty)
{
	FMessageLog(MessageLogName).Open(StaticCast<EMessageSeverity::Type>(InMinSeverity), bOpenEvenIfEmpty);
}

FMessageLogToken UMessageLogBlueprintLibrary::CreateAssetNameMessageLogToken(
	FString AssetName,
	FText OptionalLabelOverride)
{
	return FMessageLogToken::CreateAssetNameMessageLogToken(AssetName, OptionalLabelOverride);
}

FMessageLogToken UMessageLogBlueprintLibrary::CreateObjectMessageLogToken(UObject* Object, FText OptionalLabelOverride)
{
	return FMessageLogToken::CreateObjectMessageLogToken(Object, OptionalLabelOverride);
}

FMessageLogToken UMessageLogBlueprintLibrary::CreateTextMessageLogToken(FText Text)
{
	return FMessageLogToken::CreateTextMessageLogToken(Text);
}

FMessageLogToken UMessageLogBlueprintLibrary::CreateURLMessageLogToken(FString URL, FText OptionalLabelOverride)
{
	return FMessageLogToken::CreateURLMessageLogToken(URL, OptionalLabelOverride);
}
