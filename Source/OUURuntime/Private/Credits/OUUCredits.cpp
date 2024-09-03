// Copyright (c) 2024 Jonas Reich & Contributors

#include "Credits/OUUCredits.h"

#include "Algo/RandomShuffle.h"
#include "Engine/Texture2D.h"
#include "LogOpenUnrealUtilities.h"
#include "Misc/RegexUtils.h"

//---------------------------------------------------------------------------------------------------------------------
// FOUUCreditsBlock
//---------------------------------------------------------------------------------------------------------------------
void FOUUCreditsBlock::SortPeople(EOUUCreditsSortMode SortMode)
{
	switch (SortMode)
	{
	case EOUUCreditsSortMode::Random: Algo::RandomShuffle(People); break;
	case EOUUCreditsSortMode::AlphabeticalByName:
		People.Sort([](const FOUUCreditsEntry& A, const FOUUCreditsEntry& B) {
			return FText::FSortPredicate()(A.Name, B.Name);
		});
		break;
	case EOUUCreditsSortMode::KeepInputOrder: break;
	}
}

//---------------------------------------------------------------------------------------------------------------------
// UOUUCreditsLibrary
//---------------------------------------------------------------------------------------------------------------------
FOUUCredits UOUUCreditsLibrary::CreditsFromMarkdownString(const FString& MarkdownString)
{
	FOUUCredits Result;

	TArray<FString> Lines;
	MarkdownString.ParseIntoArrayLines(OUT Lines, false);
	if (Lines.Num() > 0)
	{
		Result.Blocks.AddDefaulted();
	}

	for (auto& Line : Lines)
	{
		Line = Line.TrimEnd();
		if (Line.IsEmpty())
		{
			Result.Blocks.AddDefaulted();
			continue;
		}

		if (Line.StartsWith(TEXT("![")))
		{
			auto Match =
				OUU::Runtime::RegexUtils::GetFirstRegexMatchAndGroups(TEXT("\\!\\[(.*)\\]\\((.*)\\)"), 2, Line);
			if (Match.IsValid())
			{
				// We don't do anything with alt text at the moment
				auto ImageAltText = Match.CaptureGroups[1].MatchString;
				auto ImagePath = Match.CaptureGroups[2].MatchString;
				if (auto* Image = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(ImagePath)).LoadSynchronous())
				{
					Result.Blocks.Last().Image = FSlateBrush();
					Result.Blocks.Last().Image.SetResourceObject(Image);
					Result.Blocks.Last().Image.SetImageSize(UE::Slate::FDeprecateVector2DParameter{
						static_cast<float>(Image->GetSizeX()),
						static_cast<float>(Image->GetSizeY())});
				}
				else
				{
					UE_LOG(
						LogOpenUnrealUtilities,
						Warning,
						TEXT("Image path %s could not be loaded for credits"),
						*ImagePath);
				}
				continue;
			}
			else
			{
				UE_LOG(
					LogOpenUnrealUtilities,
					Warning,
					TEXT("Line will be ignored, because it starts with '![', but could not be parsed into an image "
						 "tag: %s"),
					*Line);
				continue;
			}
		}

		const TCHAR* Stream = *Line;
		FString FirstToken;
		ensureMsgf(FParse::Token(Stream, FirstToken, true), TEXT("Expected min 1 token on non-empty line!"));

		if (FirstToken == TEXT("-"))
		{
			auto RemainingString = FString(Stream);
			FString Name, Title;
			RemainingString.Split(TEXT(","), &Name, &Title);
			if (Name.Len() > 0)
			{
				Result.Blocks.Last().People.Add(FOUUCreditsEntry{FText::FromString(Name), FText::FromString(Title)});
			}
			else
			{
				Result.Blocks.Last().People.Add(
					FOUUCreditsEntry{FText::FromString(RemainingString), FText::GetEmpty()});
			}
			continue;
		}
		if (FirstToken == TEXT(">"))
		{
			if (Result.Blocks.Last().BlockDescription.IsEmptyOrWhitespace())
			{
				Result.Blocks.Last().BlockDescription = FText::FromString(FString(Stream));
			}
			else
			{
				UE_LOG(
					LogOpenUnrealUtilities,
					Warning,
					TEXT("Markdown credits parser currently does not support multiple description lines per credits "
						 "entry. Ignoring line: %s"),
					*Line);
			}
		}

		int32 HeaderLevel = 0;
		if (FirstToken == TEXT("#"))
		{
			HeaderLevel = 1;
		}
		else if (FirstToken == TEXT("##"))
		{
			HeaderLevel = 2;
		}
		else if (FirstToken == TEXT("###"))
		{
			HeaderLevel = 3;
		}
		else
		{
			UE_LOG(
				LogOpenUnrealUtilities,
				Error,
				TEXT("Credits line will be ignored, because it starts with unexpected first token: '%s'. We expect all "
					 "non-empty lines to start with one of the following tokens: #, ##, ###, -, or > OR to be an image "
					 "tag '![](/path/to/asset)'. Ignoring line: %s"),
				*FirstToken,
				*Line);
			continue;
		}
		ensureMsgf(HeaderLevel > 0, TEXT("If we got here we expect the line to be a header"));

		// Add a new block if we have a new title.
		if (Result.Blocks.Last().BlockTitle.IsEmpty() == false)
		{
			Result.Blocks.AddDefaulted();
		}

		Result.Blocks.Last().BlockTitle =
			FText::Format(INVTEXT("<h{0}>{1}</>"), FText::AsNumber(HeaderLevel), FText::FromString(Stream));
	}

	return Result;
}
