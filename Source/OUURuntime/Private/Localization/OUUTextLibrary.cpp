// Copyright (c) 2023 Jonas Reich & Contributors

#include "Localization/OUUTextLibrary.h"

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

#undef LOCTEXT_NAMESPACE
