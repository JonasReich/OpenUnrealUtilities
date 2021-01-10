// Copyright (c) 2020 Jonas Reich

#include "SemVer/SemVerParsingStrictness.h"

FString LexToString(ESemVerParsingStrictness Strictness)
{
	switch (Strictness)
	{
	case ESemVerParsingStrictness::Strict: return "Strict";
	case ESemVerParsingStrictness::Regular: return "Regular";
	case ESemVerParsingStrictness::Liberal: return "Liberal";
	default: return "<invalid>";
	}
}
