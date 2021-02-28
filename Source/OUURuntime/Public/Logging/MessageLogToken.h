// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
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

static_assert(EMessageLogTokenType::Action == static_cast<EMessageLogTokenType>(EMessageToken::Action),
	"Action entry must match");

FString OUURUNTIME_API LexToString(EMessageLogTokenType Type);

/**
 * Message log token payload that can be specified in BP.
 * Theoretically they can be created by just setting data, but it's recommended to use the static member functions (in C++)
 * or the CreateXMessageLogToken blueprint library functions.
 */
USTRUCT(BlueprintType)
struct OUURUNTIME_API FMessageLogToken
{
	GENERATED_BODY()
public:
	static FMessageLogToken
	CreateAssetNameMessageLogToken(const FString& AssetName, const FText& OptionalLabelOverride);
	static FMessageLogToken CreateObjectMessageLogToken(UObject* Object, const FText& OptionalLabelOverride);
	static FMessageLogToken CreateTextMessageLogToken(const FText& Text);
	static FMessageLogToken CreateURLMessageLogToken(const FString& URL, const FText& OptionalLabelOverride);

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
	UObject* Object = nullptr;

	TSharedRef<IMessageToken> CreateNativeMessageToken() const;
};
