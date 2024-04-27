// Copyright (c) 2023 Jonas Reich & Contributors

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

	const TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(StaticCast<EMessageSeverity::Type>(Severity));
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

void UMessageLogBlueprintLibrary::NotifyMessageLog(
	FName MessageLogName,
	const FText& InMessage,
	EMessageLogSeverity InMinSeverity,
	bool bForce)
{
	FMessageLog(MessageLogName).Notify(InMessage, StaticCast<EMessageSeverity::Type>(InMinSeverity), bForce);
}

void UMessageLogBlueprintLibrary::NewMessageLogPage(FName MessageLogName, const FText& InLabel)
{
	FMessageLog(MessageLogName).NewPage(InLabel);
}

FMessageLogToken UMessageLogBlueprintLibrary::CreateAssetNameMessageLogToken(
	const FString& AssetName,
	const FText& OptionalLabelOverride)
{
	return FMessageLogToken::CreateAssetNameMessageLogToken(AssetName, OptionalLabelOverride);
}

FMessageLogToken UMessageLogBlueprintLibrary::CreateObjectMessageLogToken(
	const UObject* Object,
	const FText& OptionalLabelOverride)
{
	return FMessageLogToken::CreateObjectMessageLogToken(Object, OptionalLabelOverride);
}

FMessageLogToken UMessageLogBlueprintLibrary::CreateTextMessageLogToken(const FText& Text)
{
	return FMessageLogToken::CreateTextMessageLogToken(Text);
}

FMessageLogToken UMessageLogBlueprintLibrary::CreateURLMessageLogToken(const FString& URL,
	const FText& OptionalLabelOverride)
{
	return FMessageLogToken::CreateURLMessageLogToken(URL, OptionalLabelOverride);
}
