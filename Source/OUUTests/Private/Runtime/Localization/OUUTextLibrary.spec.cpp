// Copyright (c) 2023 Jonas Reich & Contributors

#include "Localization/ScopedCultureOverride.h"
#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

	#include "LocalizationModule.h"
	#include "Localization/OUUTextLibrary.h"

	#define OUU_TEST_CATEGORY OpenUnrealUtilities.Runtime.Localization
	#define OUU_TEST_TYPE	  TextLibrary

//////////////////////////////////////////////////////////////////////////
// Normally, I would have implemented all test cases as spec test with individual It blocks, but the fact that this
// deals with FTexts and requires overriding and restoring the Engine culture makes it too time-consuming to actually
// implement them all separately with unique initialization/cleanup.
//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(FormatListText, DEFAULT_OUU_TEST_FLAGS)
{
	FScopedCultureOverride ScopedCultureOverride{TEXT("en")};

	// with last 'and' separator enabled
	{
		// It should return the input for a single word
		{
			const auto Result = UOUUTextLibrary::FormatListText({INVTEXT("foo")}, true);
			const auto ResultAsString = Result.ToString();
			SPEC_TEST_EQUAL(ResultAsString, FString(TEXT("foo")));
		};

		// It should combine the words with 'and'
		{
			const auto Result = UOUUTextLibrary::FormatListText({INVTEXT("foo"), INVTEXT("bar")}, true);
			const auto ResultAsString = Result.ToString();
			SPEC_TEST_EQUAL(ResultAsString, FString(TEXT("foo and bar")));
		}

		// It should only combine the last words with 'and' (3 words)
		{
			const auto Result =
				UOUUTextLibrary::FormatListText({INVTEXT("foo"), INVTEXT("bar"), INVTEXT("foobar")}, true);
			const auto ResultAsString = Result.ToString();
			SPEC_TEST_EQUAL(ResultAsString, FString(TEXT("foo, bar and foobar")));
		}

		// It should only combine the last words with 'and' (4 words)
		{
			const auto Result = UOUUTextLibrary::FormatListText(
				{INVTEXT("foo"), INVTEXT("bar"), INVTEXT("foobar"), INVTEXT("foobarbar")},
				true);
			const auto ResultAsString = Result.ToString();
			SPEC_TEST_EQUAL(ResultAsString, FString(TEXT("foo, bar, foobar and foobarbar")));
		}
	}

	// with last 'and' separator disabled
	{
		// It should return the input for a single word
		{
			const auto Result = UOUUTextLibrary::FormatListText({INVTEXT("foo")}, false);
			const auto ResultAsString = Result.ToString();
			SPEC_TEST_EQUAL(ResultAsString, FString(TEXT("foo")));
		};

		// It should NOT combine the words with 'and'
		{
			const auto Result = UOUUTextLibrary::FormatListText({INVTEXT("foo"), INVTEXT("bar")}, false);
			const auto ResultAsString = Result.ToString();
			SPEC_TEST_EQUAL(ResultAsString, FString(TEXT("foo, bar")));
		}

		// It should NOT combine any words with 'and' (3 words)
		{
			const auto Result =
				UOUUTextLibrary::FormatListText({INVTEXT("foo"), INVTEXT("bar"), INVTEXT("foobar")}, false);
			const auto ResultAsString = Result.ToString();
			SPEC_TEST_EQUAL(ResultAsString, FString(TEXT("foo, bar, foobar")));
		}

		// It should NOT combine any words with 'and' (4 words)
		{
			const auto Result = UOUUTextLibrary::FormatListText(
				{INVTEXT("foo"), INVTEXT("bar"), INVTEXT("foobar"), INVTEXT("foobarbar")},
				false);
			const auto ResultAsString = Result.ToString();
			SPEC_TEST_EQUAL(ResultAsString, FString(TEXT("foo, bar, foobar, foobarbar")));
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(FormatListText_GenericSeparator, DEFAULT_OUU_TEST_FLAGS)
{
	FScopedCultureOverride ScopedCultureOverride{TEXT("en")};

	// It should combine two texts with a generic separator
	const auto Result = UOUUTextLibrary::FormatListText_GenericSeparator(INVTEXT("foo"), INVTEXT("bar"));
	const auto ResultAsString = Result.ToString();
	SPEC_TEST_EQUAL(ResultAsString, FString(TEXT("foo, bar")));

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(FormatListText_FinalAndSeparator, DEFAULT_OUU_TEST_FLAGS)
{
	FScopedCultureOverride ScopedCultureOverride{TEXT("en")};

	// It should combine two texts with a special separator (default: 'and')
	const auto Result = UOUUTextLibrary::FormatListText_FinalAndSeparator(INVTEXT("foo"), INVTEXT("bar"));
	const auto ResultAsString = Result.ToString();
	SPEC_TEST_EQUAL(ResultAsString, FString(TEXT("foo and bar")));

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(JoinBy, DEFAULT_OUU_TEST_FLAGS)
{
	FScopedCultureOverride ScopedCultureOverride{TEXT("en")};

	// It should combine two texts with a provided separator
	const auto Result =
		UOUUTextLibrary::JoinBy(TArray<FText>{INVTEXT("foo"), INVTEXT("bar"), INVTEXT("foobar")}, INVTEXT("&+and+&"));
	const auto ResultAsString = Result.ToString();
	SPEC_TEST_EQUAL(ResultAsString, FString(TEXT("foo&+and+&bar&+and+&foobar")));

	return true;
}

//////////////////////////////////////////////////////////////////////////

#endif
