// Copyright (c) 2023 Jonas Reich & Contributors

#include "GameplayTags/GameplayTagQueryParser.h"

#include "Templates/StringUtils.h"

/** Internal helper functions for query parsing. Should not be required outside. */
namespace OUU::Runtime::Private::GameplayTagQueryParser
{
	/**
	 * Start strings for operators.
	 * Contain the opening parentheses so we require slightly less code to find them.
	 * The disadvantage is that strings like "ANY (Foo, Bar)" are not parsed, because of the additional space.
	 */
	namespace OpStrings
	{
		FString Any = "ANY(";
		FString All = "ALL(";
		FString None = "NONE(";
	} // namespace OpStrings

	// ----------------

	void SkipWhitespace(const TCHAR*& StrRef)
	{
		while (FChar::IsWhitespace(*StrRef))
		{
			StrRef++;
		}
	}

	void SkipCommasAndWhitespace(const TCHAR*& StrRef)
	{
		while (*StrRef == LITERAL(TCHAR, ',') || FChar::IsWhitespace(*StrRef))
		{
			StrRef++;
		}
	}

	/**
	 * Returns true if the main string contains the compare string.
	 * Modifies the main string pointer to point to the character after the compare string
	 */
	bool SkipIfStartsWith(const TCHAR*& StrRef, const TCHAR* CmpStr)
	{
		if (FCString::Strnicmp(StrRef, CmpStr, FCString::Strlen(CmpStr)) == 0)
		{
			StrRef += FCString::Strlen(CmpStr);
			return true;
		}
		return false;
	}

	/** Checks if the string starts with any of the OpStrings. Does not modify the string pointer */
	bool StartsWithArbitraryOp(const TCHAR* Str)
	{
		return SkipIfStartsWith(Str, *OpStrings::Any) || SkipIfStartsWith(Str, *OpStrings::All)
			|| SkipIfStartsWith(Str, *OpStrings::None);
	}

	// ----------------

	/**
	 * For brevity this type is just called FQueryExprHelper, but a more verbose name would be
	 * FGameplayTagQueryExpressionConstructionHelper.
	 * It's a utility class that maps to the query expressions of gameplay tag queries and helps building a tree of
	 * these expressions. Expressions can target tags or a nested set of other expressions.
	 *
	 * This structure simplifies this approach by using the same type for both kinds of operations.
	 * The matching native expression is resolved dynamically based on the nature of the nested contents.
	 * e.g.
	 *     - "ALL(OR(), ALL())" has nested expressions and must be resolved to
	 * FGameplayTagQueryExpression::AllExprMatch()
	 *     - "ALL(Foo.Bar, Alpha)" has nested tags and must be resolved to FGameplayTagQueryExpression::AllTagsMatch()
	 *
	 * This dynamic resolution is realized via child classes for each of the individual operators (all, any, none).
	 */
	struct FQueryExprHelper
	{
		virtual ~FQueryExprHelper() {}

	private:
		/**
		 * Nested expressions, if any.
		 * Should never have elements if there are Tags.
		 */
		TArray<TSharedPtr<FQueryExprHelper>> InnerExpressions;

		/**
		 * Nested tags, if any.
		 * Should never have elements if there are InnerExprs
		 */
		TArray<FGameplayTag> Tags;

		/** Instance of a native expression corresponding to this wrapper */
		TSharedPtr<FGameplayTagQueryExpression> NativeExpr;

	public:
		/** Initialize the wrapped native expression so it expects tags */
		virtual void InitNativeExpressionForTags(TSharedRef<FGameplayTagQueryExpression> InNativeExpr) = 0;

		/** Initialize the wrapped native expression so it expects nested expressions */
		virtual void InitNativeExpressionForExpressions(TSharedRef<FGameplayTagQueryExpression> InNativeExpr) = 0;

		/**
		 * Get the OpString corresponding to this expression type.
		 * Must be implemented by child classes and should be implemented as type static.
		 */
		virtual FString GetOpString() const = 0;

		/** Create a query expression tree from a string. */
		static TSharedPtr<FQueryExprHelper> FromString(const TCHAR*& Str);

		/** Convert this query expression and its subtree into a valid/parseable query string. */
		FString ToString() const
		{
			const FString InnerExpressionsString = FString::JoinBy(InnerExpressions, TEXT(", "), [](auto& Element) {
				return FString::Printf(TEXT("%s"), *FString(Element ? Element->ToString() : ""));
			});
			const FString TagsString = FString::JoinBy(Tags, TEXT(", "), [](auto& Element) {
				return FString::Printf(TEXT("%s"), *LexToString(Element));
			});

			return FString::Printf(TEXT("%s%s%s)"), *GetOpString(), *InnerExpressionsString, *TagsString);
		}

		void AddInnerExpression(TSharedPtr<FQueryExprHelper> InExpression) { InnerExpressions.Add(InExpression); }

		void AddTag(FGameplayTag&& Tag) { Tags.Add(Tag); }

		/** Convert helper structs into a tree of FGameplayTagQueryExpressions */
		FGameplayTagQueryExpression& MakeNativeExpr()
		{
			NativeExpr = MakeShared<FGameplayTagQueryExpression>();
			if (Tags.Num() > 0)
			{
				InitNativeExpressionForTags(NativeExpr.ToSharedRef());
				for (auto& Tag : Tags)
				{
					NativeExpr->AddTag(Tag);
				}
			}
			else
			{
				InitNativeExpressionForExpressions(NativeExpr.ToSharedRef());
				for (auto& Expr : InnerExpressions)
				{
					NativeExpr->AddExpr(Expr->MakeNativeExpr());
				}
			}
			return *NativeExpr;
		}
	};

	/** FQueryExprHelper subclass for ANY() operations */
	struct FQueryExpr_AnyExpr : FQueryExprHelper
	{
		virtual void InitNativeExpressionForTags(TSharedRef<FGameplayTagQueryExpression> InNativeExpr) override
		{
			InNativeExpr->AnyTagsMatch();
		}

		virtual void InitNativeExpressionForExpressions(TSharedRef<FGameplayTagQueryExpression> InNativeExpr) override
		{
			InNativeExpr->AnyExprMatch();
		}

		virtual FString GetOpString() const override { return OpStrings::Any; }
	};

	/** FQueryExprHelper subclass for ALL() operations */
	struct FQueryExpr_AllExpr : FQueryExprHelper
	{
		virtual void InitNativeExpressionForTags(TSharedRef<FGameplayTagQueryExpression> InNativeExpr) override
		{
			InNativeExpr->AllTagsMatch();
		}

		virtual void InitNativeExpressionForExpressions(TSharedRef<FGameplayTagQueryExpression> InNativeExpr) override
		{
			InNativeExpr->AllExprMatch();
		}

		virtual FString GetOpString() const override { return OpStrings::All; }
	};

	/** FQueryExprHelper subclass for NONE() operations */
	struct FQueryExpr_NoExpr : FQueryExprHelper
	{
		virtual void InitNativeExpressionForTags(TSharedRef<FGameplayTagQueryExpression> InNativeExpr) override
		{
			InNativeExpr->NoTagsMatch();
		}

		virtual void InitNativeExpressionForExpressions(TSharedRef<FGameplayTagQueryExpression> InNativeExpr) override
		{
			InNativeExpr->NoExprMatch();
		}

		virtual FString GetOpString() const override { return OpStrings::None; }
	};

	/**
	 * Implementation for FQueryExprHelper::FromString (see declaration above).
	 * This is required to be defined after the declaration of the FQueryExprHelper subclasses,
	 * so it's extracted to this location.
	 */
	TSharedPtr<FQueryExprHelper> FQueryExprHelper::FromString(const TCHAR*& Str)
	{
		if (SkipIfStartsWith(Str, *OpStrings::Any))
			return MakeShared<FQueryExpr_AnyExpr>();
		if (SkipIfStartsWith(Str, *OpStrings::All))
			return MakeShared<FQueryExpr_AllExpr>();
		if (SkipIfStartsWith(Str, *OpStrings::None))
			return MakeShared<FQueryExpr_NoExpr>();
		return nullptr;
	}

	/** Forward declare utility function to parse inner expressions from the string */
	void ParseInnerExprs(const TCHAR*& Str, TSharedPtr<FQueryExprHelper> HelperRoot);
	/** Forward declare utility function to parse inner tags from the string */
	void ParseTags(const TCHAR*& Str, TSharedPtr<FQueryExprHelper> HelperRoot);

	/**
	 * Parse a string into a tree of helper structs.
	 * This function does the bulk of the actual parsing work (separating operators, iterating tags, etc).
	 */
	TSharedPtr<FQueryExprHelper> ParseIntoHelperTree(const TCHAR*& Str)
	{
		// Skip all whitespace before the first operation
		SkipWhitespace(Str);

		// Create the root expression from string
		auto HelperRoot = FQueryExprHelper::FromString(Str);

		// Skip all whitespace after the opening parentheses
		SkipWhitespace(Str);

		// Parse the nested content of the expression - either inner expressions or gameplay tags
		if (StartsWithArbitraryOp(Str))
		{
			ParseInnerExprs(Str, HelperRoot);
		}
		else
		{
			ParseTags(Str, HelperRoot);
		}
		return HelperRoot;
	}

	void ParseInnerExprs(const TCHAR*& Str, TSharedPtr<FQueryExprHelper> HelperRoot)
	{
		// If we have a valid expression operation string now, this expression is an ExprExpr
		// -> an expression wrapping other expressions

		// Skip the first check of the loop -> already done in the surrounding if-block
		do
		{
			// Continue parsing expressions and their nested trees until no other expression is found at the start of
			// the string
			HelperRoot->AddInnerExpression(ParseIntoHelperTree(Str));
			// Skip commas between operations
			SkipCommasAndWhitespace(Str);
		} while (StartsWithArbitraryOp(Str));
		UE_CLOG(*Str != LITERAL(TCHAR, ')'), LogTemp, Warning, TEXT("Parentheses do not close properly!"));
	}

	void ParseTags(const TCHAR*& Str, TSharedPtr<FQueryExprHelper> HelperRoot)
	{
		const TCHAR* AllTagsEnd = Str;
		while (AllTagsEnd)
		{
			if (*AllTagsEnd == LITERAL(TCHAR, ')'))
				break;
			++AllTagsEnd;
		}
		if (AllTagsEnd)
		{
			++AllTagsEnd;
			const TCHAR* TagStart = Str;
			const TCHAR* TagEnd = Str;
			while (TagEnd < AllTagsEnd)
			{
				// Use both comma and space as a valid delimiter for tags
				// End of tag list is always a ')'
				if (*TagEnd == LITERAL(TCHAR, ',') || *TagEnd == LITERAL(TCHAR, ' ') || *TagEnd == LITERAL(TCHAR, ')'))
				{
					if (TagStart >= TagEnd)
						break;

					FString TagString{StaticCast<int32>(TagEnd - TagStart), TagStart};
					constexpr bool bErrorIfNotFound = false;
					HelperRoot->AddTag(FGameplayTag::RequestGameplayTag(*TagString, bErrorIfNotFound));

					// Skip commas between tags
					SkipCommasAndWhitespace(TagEnd);
					// Continue with next tag
					TagStart = TagEnd;

					if (*TagEnd == LITERAL(TCHAR, ')'))
					{
						++TagEnd;
						break;
					}
				}
				else
				{
					++TagEnd;
				}
			}
			Str = TagEnd;
		}
	}
} // namespace OUU::Runtime::Private::GameplayTagQueryParser

FGameplayTagQuery FGameplayTagQueryParser::ParseQuery(const FString& SourceString)
{
	namespace QueryParser = OUU::Runtime::Private::GameplayTagQueryParser;

	// Declare replacement string on outer scope so we can assign it to Str without going out of scope
	FString ReplacementString;
	const TCHAR* Str = *SourceString;
	QueryParser::SkipWhitespace(Str);
	if (!QueryParser::StartsWithArbitraryOp(Str))
	{
		// If no root query operation is used, assume it's an ALL(Tag) query,
		// e.g. "Foo.Bar" should just be interpreted as "ALL(Foo.Bar)"
		ReplacementString = QueryParser::OpStrings::Any + SourceString + ")";
		Str = *ReplacementString;
	}

	if (auto HelperTree = QueryParser::ParseIntoHelperTree(Str))
	{
		auto& NativeExpr = HelperTree->MakeNativeExpr();
		auto Result = FGameplayTagQuery::BuildQuery(NativeExpr, SourceString);
		return Result;
	}
	else
	{
		return FGameplayTagQuery::EmptyQuery;
	}
}
