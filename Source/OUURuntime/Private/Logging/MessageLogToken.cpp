// Copyright (c) 2023 Jonas Reich & Contributors

#include "Logging/MessageLogToken.h"

#include "Localization/OUUTextLibrary.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"

FString LexToString(EMessageLogTokenType Type)
{
	switch (Type)
	{
#define MESSAGELOG_TOKEN_TYPE_TO_STRING(Entry)                                                                         \
	case EMessageLogTokenType::Entry: return PREPROCESSOR_TO_STRING(Entry);
		MESSAGELOG_TOKEN_TYPE_TO_STRING(Action)
		MESSAGELOG_TOKEN_TYPE_TO_STRING(AssetName)
		MESSAGELOG_TOKEN_TYPE_TO_STRING(Documentation)
		MESSAGELOG_TOKEN_TYPE_TO_STRING(Image)
		MESSAGELOG_TOKEN_TYPE_TO_STRING(Object)
		MESSAGELOG_TOKEN_TYPE_TO_STRING(Severity)
		MESSAGELOG_TOKEN_TYPE_TO_STRING(Text)
		MESSAGELOG_TOKEN_TYPE_TO_STRING(Tutorial)
		MESSAGELOG_TOKEN_TYPE_TO_STRING(URL)
		MESSAGELOG_TOKEN_TYPE_TO_STRING(EdGraph)
		MESSAGELOG_TOKEN_TYPE_TO_STRING(DynamicText)
	default: return "<invalid>";
#undef MESSAGELOG_TOKEN_TYPE_TO_STRING
	}
}

FMessageLogToken FMessageLogToken::CreateAssetNameMessageLogToken(
	const FString& AssetName,
	const FText& OptionalLabelOverride)
{
	FMessageLogToken Result;
	Result.Type = EMessageLogTokenType::AssetName;
	Result.AssetName = AssetName;
	Result.Text = OptionalLabelOverride;
	return Result;
}

FMessageLogToken FMessageLogToken::CreateObjectMessageLogToken(
	const UObject* Object,
	const FText& OptionalLabelOverride)
{
	FMessageLogToken Result;
	Result.Type = EMessageLogTokenType::Object;
	Result.Object = Object;
	Result.Text = OptionalLabelOverride;
	return Result;
}

FMessageLogToken FMessageLogToken::CreateTextMessageLogToken(const FText& Text)
{
	FMessageLogToken Result;
	Result.Type = EMessageLogTokenType::Text;
	Result.Text = Text;
	return Result;
}

FMessageLogToken FMessageLogToken::CreateURLMessageLogToken(const FString& URL, const FText& OptionalLabelOverride)
{
	FMessageLogToken Result;
	Result.Type = EMessageLogTokenType::URL;
	Result.URL = URL;
	Result.Text = OptionalLabelOverride;
	return Result;
}

FMessageLogToken FMessageLogToken::Create(const TSharedRef<IMessageToken>& NativeToken)
{
	FMessageLogToken Result;
	Result.Type = EMessageLogTokenType::ForwardNativeToken;
	Result.ForwardedNativeToken = NativeToken;
	return Result;
}

FText FMessageLogToken::ListAsText(const TArray<FMessageLogToken>& MessageTokenList)
{
	TArray<FText> TokensAsText;
	for (auto& Token : MessageTokenList)
	{
		TokensAsText.Add(Token.CreateNativeMessageToken()->ToText());
	}
	return UOUUTextLibrary::JoinBy(TokensAsText, INVTEXT(" "));
}

TSharedRef<IMessageToken> FMessageLogToken::CreateNativeMessageToken() const
{
	switch (Type)
	{
	case EMessageLogTokenType::AssetName: return FAssetNameToken::Create(AssetName, Text);
	case EMessageLogTokenType::Object: return FUObjectToken::Create(Object, Text);
	case EMessageLogTokenType::Text: return FTextToken::Create(Text);
	case EMessageLogTokenType::URL: return FURLToken::Create(URL, Text);
	case EMessageLogTokenType::ForwardNativeToken: return ForwardedNativeToken.ToSharedRef();
	default:
		FMessageLog("Blueprint")
			.Error(FText::FromString(
				FString::Printf(TEXT("MessageLogToken cannot be created with type %s"), *LexToString(Type))));
	}
	return FTextToken::Create(FText::FromString(TEXT("")));
}
