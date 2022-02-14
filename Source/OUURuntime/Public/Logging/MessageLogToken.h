// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Logging/TokenizedMessage.h"

#include "MessageLogToken.generated.h"

/**
 * Blueprint exposed copy of EMessageToken
 */
UENUM(BlueprintType)
enum class EMessageLogTokenType : uint8
{
	// supported in BP: no
	Action = 0,
	// supported in BP: yes
	AssetName = EMessageToken::AssetName,
	// supported in BP: no
	Documentation = EMessageToken::Documentation,
	// supported in BP: no
	Image = EMessageToken::Image,
	// supported in BP: yes
	Object = EMessageToken::Object,
	// supported in BP: no
	Severity = EMessageToken::Severity,
	// supported in BP: yes
	Text = EMessageToken::Text,
	// supported in BP: no
	Tutorial = EMessageToken::Tutorial,
	// supported in BP: yes
	URL = EMessageToken::URL,
	// supported in BP: no
	EdGraph = EMessageToken::EdGraph,
	// supported in BP: no
	DynamicText = EMessageToken::DynamicText
};

static_assert(
	EMessageLogTokenType::Action == static_cast<EMessageLogTokenType>(EMessageToken::Action),
	"Action entry must match");

FString OUURUNTIME_API LexToString(EMessageLogTokenType Type);

/**
 * Message log token payload that can be specified in BP.
 * Theoretically they can be created by just setting data, but it's recommended to use the static member functions (in
 * C++) or the CreateXMessageLogToken blueprint library functions.
 */
USTRUCT(BlueprintType)
struct OUURUNTIME_API FMessageLogToken
{
	GENERATED_BODY()
public:
	static FMessageLogToken CreateAssetNameMessageLogToken(
		const FString& AssetName,
		const FText& OptionalLabelOverride);
	static FMessageLogToken CreateObjectMessageLogToken(const UObject* Object, const FText& OptionalLabelOverride);
	static FMessageLogToken CreateTextMessageLogToken(const FText& Text);
	static FMessageLogToken CreateURLMessageLogToken(const FString& URL, const FText& OptionalLabelOverride);

	static FORCEINLINE FMessageLogToken Create(FText Text) { return CreateTextMessageLogToken(Text); }

	static FORCEINLINE FMessageLogToken Create(FString Text)
	{
		return CreateTextMessageLogToken(FText::FromString(Text));
	}

	static FORCEINLINE FMessageLogToken Create(const UObject* Object)
	{
		return CreateObjectMessageLogToken(Object, FText());
	}

	static FORCEINLINE FMessageLogToken Create(FMessageLogToken Token) { return Token; }

	template <typename FirstType>
	static TArray<FMessageLogToken> CreateList(FirstType FirstArgument)
	{
		return TArray<FMessageLogToken>{Create(FirstArgument)};
	}

	template <typename FirstType, typename... RemainingTypes>
	static TArray<FMessageLogToken> CreateList(FirstType FirstArgument, RemainingTypes... RemainingArguments)
	{
		TArray<FMessageLogToken> ResultArray{Create(FirstArgument)};
		ResultArray.Append(CreateList(RemainingArguments...));
		return ResultArray;
	}

	// Which of the payload data should be used
	UPROPERTY(BlueprintReadWrite)
	EMessageLogTokenType Type = EMessageLogTokenType::Text;

	UPROPERTY(BlueprintReadWrite)
	FText Text = {};

	UPROPERTY(BlueprintReadWrite)
	FString URL = "";

	UPROPERTY(BlueprintReadWrite)
	FString AssetName = "";

	UPROPERTY(BlueprintReadWrite)
	const UObject* Object = nullptr;

	TSharedRef<IMessageToken> CreateNativeMessageToken() const;
};
