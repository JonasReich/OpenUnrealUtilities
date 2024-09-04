// Copyright (c) 2024 Jonas Reich & Contributors

#include "Misc/OUUPathUtils.h"

namespace OUU::Runtime::PathUtils
{
	FString SanitizeAsciiFileName(FStringView FileName)
	{
		static constexpr FAsciiSet ValidAsciiSet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-";
		FString Result;
		while (true)
		{
			FStringView ValidPrefix = FAsciiSet::FindPrefixWith(FileName, ValidAsciiSet);
			Result.Append(ValidPrefix);
			FileName.RemovePrefix(ValidPrefix.Len());

			if (FileName.IsEmpty())
			{
				break;
			}

			FileName.RemovePrefix(1);
		}
		return Result;
	}
} // namespace OUU::Runtime::PathUtils
