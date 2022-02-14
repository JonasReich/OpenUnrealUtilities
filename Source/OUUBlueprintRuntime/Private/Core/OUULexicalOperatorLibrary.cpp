// Copyright (c) 2022 Jonas Reich

#include "Core/OUULexicalOperatorLibrary.h"

bool UOUULexicalOperatorLibrary::Less_StringString(const FString& A, const FString& B)
{
	return A < B;
}

bool UOUULexicalOperatorLibrary::Greater_StringString(const FString& A, const FString& B)
{
	return A > B;
}

bool UOUULexicalOperatorLibrary::LessEqual_StringString(const FString& A, const FString& B)
{
	return A <= B;
}

bool UOUULexicalOperatorLibrary::GreaterEqual_StringString(const FString& A, const FString& B)
{
	return A >= B;
}

bool UOUULexicalOperatorLibrary::Less_NameName(const FName& A, const FName& B)
{
	return FNameLexicalLess().operator()(A, B);
}

bool UOUULexicalOperatorLibrary::Greater_NameName(const FName& A, const FName& B)
{
	return !LessEqual_NameName(A, B);
}

bool UOUULexicalOperatorLibrary::LessEqual_NameName(const FName& A, const FName& B)
{
	return (A == B) || Less_NameName(A, B);
}

bool UOUULexicalOperatorLibrary::GreaterEqual_NameName(const FName& A, const FName& B)
{
	return (A == B) || !Less_NameName(A, B);
}
