// Copyright (c) 2021 Jonas Reich

#include "SemVer/PreReleaseIdentifier.h"
#include "Misc/RegexUtils.h"

FSemVerPreReleaseIdentifier::FSemVerPreReleaseIdentifier(const FString& SourceString, ESemVerParsingStrictness InStrictness /*= ESemVerParsingStrictness::Strict*/)
{
	TryParseString(SourceString, InStrictness);
}

FString FSemVerPreReleaseIdentifier::ToString() const
{
	return FString::Join(Identifiers, TEXT("."));
}

bool FSemVerPreReleaseIdentifier::TryParseString(const FString& SourceString, ESemVerParsingStrictness InStrictness)
{
	if (SourceString.Len() == 0)
	{
		Strictness = ESemVerParsingStrictness::Strict;
		Identifiers.Empty();
		return true;
	}

	Strictness = InStrictness;
	SourceString.ParseIntoArray(Identifiers, TEXT("."));
	bool bIdentifiersOk = true;

	for (auto& Identifier : Identifiers)
	{
		if (InStrictness != ESemVerParsingStrictness::Strict)
		{
			if (Identifier.IsEmpty())
			{
				return false;
			}

			if (Identifier.IsNumeric())
			{
				bool bHasRemovedAZero = false;

				// Strip leading zeroes to comply with strict standard afterwards
				while (Identifier.RemoveFromStart("0"))
				{
					bHasRemovedAZero = true;
				}

				if (bHasRemovedAZero && Identifier.IsEmpty())
				{
					Identifier = "0";
				}
			}
		}

		if (!FRegexUtils::MatchesRegexExact("^(?:0|[1-9]\\d*|\\d*[a-zA-Z-][0-9a-zA-Z-]*)$", Identifier))
		{
			bIdentifiersOk = false;
			break;
		}
	}

	Identifiers.RemoveAll([](auto& Identifier) -> bool
	{
		return Identifier.IsEmpty();
	});
	
	if (bIdentifiersOk)
	{
		return true;
	}

	Strictness = ESemVerParsingStrictness::Strict;
	Identifiers.Empty();
	return false;
}

const TArray<FString> FSemVerPreReleaseIdentifier::GetIdentifiers()
{
	return Identifiers;
}

bool FSemVerPreReleaseIdentifier::TryIncrement()
{
	int32 NumIdentifiers = Identifiers.Num();
	if (NumIdentifiers == 0)
	{
		Identifiers.Add("1");
		return true;
	}

	FString& LastIdentifier = Identifiers.Last();
	int32 LastIdentiferAsNumeric = TryParseNumericIdentifier(LastIdentifier);
	if (LastIdentiferAsNumeric != INDEX_NONE)
	{
		LastIdentiferAsNumeric++;
		LastIdentifier = LexToString(LastIdentiferAsNumeric);
		return true;
	}

	return false;
}

bool FSemVerPreReleaseIdentifier::operator==(const FSemVerPreReleaseIdentifier& Other) const
{
	int32 ThisNumIdentifiers = Identifiers.Num();
	if (ThisNumIdentifiers != Other.Identifiers.Num())
	{
		return false;
	}

	for (int32 i = 0; i < ThisNumIdentifiers; i++)
	{
		const FString ThisIdentifier = Identifiers[i];
		const FString OtherIdentifier = Other.Identifiers[i];

		if (ThisIdentifier != OtherIdentifier)
		{
			return false;
		}
	}

	return true;
}

bool FSemVerPreReleaseIdentifier::operator!=(const FSemVerPreReleaseIdentifier& Other) const
{
	return !((*this) == Other);
}

bool FSemVerPreReleaseIdentifier::operator<(const FSemVerPreReleaseIdentifier& Other) const
{
	int32 ThisNumIdentifiers = Identifiers.Num();
	int32 OtherNumIdentifiers = Other.Identifiers.Num();

	int32 MinNumIdentifiers = FMath::Min(ThisNumIdentifiers, OtherNumIdentifiers);

	for (int32 i = 0; i < MinNumIdentifiers; i++)
	{
		const FString ThisIdentifier = Identifiers[i];
		const FString OtherIdentifier = Other.Identifiers[i];

		if (ThisIdentifier == OtherIdentifier)
		{
			continue;
		}

		return CompareStringIdentifiersSmaller(ThisIdentifier, OtherIdentifier);
	}

	// More identifiers means lower precendece
	return ThisNumIdentifiers > OtherNumIdentifiers;
}

bool FSemVerPreReleaseIdentifier::operator<=(const FSemVerPreReleaseIdentifier& Other) const
{
	return !((*this) > Other);
}

bool FSemVerPreReleaseIdentifier::operator>(const FSemVerPreReleaseIdentifier& Other) const
{
	int32 ThisNumIdentifiers = Identifiers.Num();
	int32 OtherNumIdentifiers = Other.Identifiers.Num();

	int32 MinNumIdentifiers = FMath::Min(ThisNumIdentifiers, OtherNumIdentifiers);

	for (int32 i = 0; i < MinNumIdentifiers; i++)
	{
		const FString ThisIdentifier = Identifiers[i];
		const FString OtherIdentifier = Other.Identifiers[i];

		if (ThisIdentifier == OtherIdentifier)
		{
			continue;
		}

		return CompareStringIdentifiersBigger(ThisIdentifier, OtherIdentifier);
	}

	// More identifiers means lower precendece
	return ThisNumIdentifiers < OtherNumIdentifiers;
}

bool FSemVerPreReleaseIdentifier::operator>=(const FSemVerPreReleaseIdentifier& Other) const
{
	return !((*this) < Other);
}

bool FSemVerPreReleaseIdentifier::CompareStringIdentifiersSmaller(const FString& A, const FString& B)
{
	int32 AInt = TryParseNumericIdentifier(A);
	int32 BInt = TryParseNumericIdentifier(B);
	if (AInt != INDEX_NONE && BInt != INDEX_NONE)
	{
		return AInt < BInt;
	}
	if (AInt != INDEX_NONE && BInt == INDEX_NONE)
	{
		return true;
	}
	if (BInt != INDEX_NONE && AInt == INDEX_NONE)
	{
		return false;
	}
	return A < B;
}

bool FSemVerPreReleaseIdentifier::CompareStringIdentifiersBigger(const FString& A, const FString& B)
{
	int32 AInt = TryParseNumericIdentifier(A);
	int32 BInt = TryParseNumericIdentifier(B);
	if (AInt != INDEX_NONE && BInt != INDEX_NONE)
	{
		return AInt > BInt;
	}
	if (AInt != INDEX_NONE && BInt == INDEX_NONE)
	{
		return false;
	}
	if (BInt != INDEX_NONE && AInt == INDEX_NONE)
	{
		return true;
	}
	return A > B;
}

int32 FSemVerPreReleaseIdentifier::TryParseNumericIdentifier(const FString& Identifier)
{
	if(Identifier.IsNumeric())
	{
		int32 Result = 0;
		LexFromString(Result, *Identifier);
		return Result;
	}
	return INDEX_NONE;
}
