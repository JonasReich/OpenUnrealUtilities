// Copyright (c) 2022 Jonas Reich

#include "Localization/OUUTextLibrary.h"

#define LOCTEXT_NAMESPACE "OUUTextLibrary"

FText UOUUTextLibrary::FormatListText(const TArray<FText>& Texts, bool bFormatWithFinalAndSeparator)
{
	const int32 TextsNum = Texts.Num();
	switch (TextsNum)
	{
	case 0: return FText();
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

#undef LOCTEXT_NAMESPACE
