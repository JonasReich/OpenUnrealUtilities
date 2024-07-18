// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"
#include "OUUCredits.generated.h"

UENUM(BlueprintType)
enum class EOUUCreditsSortMode : uint8
{
	KeepInputOrder,
	AlphabeticalByName,
	Random
};

USTRUCT(BlueprintType)
struct FOUUCreditsEntry
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite)
	FText Name;
	UPROPERTY(BlueprintReadWrite)
	FText Title;
};

USTRUCT(BlueprintType)
struct FOUUCreditsBlock
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite)
	FText BlockTitle;
	UPROPERTY(BlueprintReadWrite)
	FSlateBrush Image;
	UPROPERTY(BlueprintReadWrite)
	FText BlockDescription;
	UPROPERTY(BlueprintReadWrite)
	TArray<FOUUCreditsEntry> People;

	void SortPeople(EOUUCreditsSortMode SortMode);
};

USTRUCT(BlueprintType)
struct FOUUCredits
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite)
	TArray<FOUUCreditsBlock> Blocks;
};

UCLASS()
class UOUUCreditsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	static FOUUCredits CreditsFromMarkdownString(const FString& MarkdownString);
};
