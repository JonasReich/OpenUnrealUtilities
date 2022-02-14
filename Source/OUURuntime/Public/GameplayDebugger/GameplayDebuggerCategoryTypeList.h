// Copyright (c) 2022 Jonas Reich

#pragma once

#if WITH_GAMEPLAY_DEBUGGER

	#include "GameplayDebugger.h"
	#include "GameplayDebuggerCategory.h"
	#include "Templates/Models.h"
	#include "Templates/UnrealTypeTraits.h"

/**
 * Concept types to use for static type checks in TGameplayDebuggerCategoryTraits.
 * Used to check if the required functions exist and return the correct types.
 */
namespace GameplayDebuggerCategoryConcepts
{
	struct CHasCategoryName
	{
		template <typename T>
		auto Requires() -> decltype(FName(T::GetCategoryName()));
	};

	struct CHasExplicitInstanceMethod
	{
		template <typename T>
		auto Requires() -> decltype(TSharedRef<FGameplayDebuggerCategory>(T::MakeInstance()));
	};
} // namespace GameplayDebuggerCategoryConcepts

/**
 * Trait class for gameplay debugger categories that simplifies category registration and de-registration.
 * You don't have to include this file in header files of categories themselves, but there are two
 * special member functions you must consider:
 *
 * static constexpr auto GetCategoryName()
 *     --> returning the category name as string literal.
 *     --> MUST be implemented by debugger categories.
 * static TSharedRef<FGameplayDebuggerCategory> MakeInstance()
 *     --> returning an instance of the category.
 *     --> MAY be implemented by debugger categories.
 *         If not present, the default implementation below will be used.
 */
template <class GameplayDebuggerCategoryType>
struct TGameplayDebuggerCategoryTraits
{
public:
	static_assert(
		TIsDerivedFrom<GameplayDebuggerCategoryType, FGameplayDebuggerCategory>::Value,
		"GameplayDebuggerCategoryType must be derived from FGameplayDebuggerCategory");
	static_assert(
		TModels<GameplayDebuggerCategoryConcepts::CHasCategoryName, GameplayDebuggerCategoryType>::Value,
		"GameplayDebuggerCategoryType must have a static constexpr member function called GetCategoryName that "
		"returns a string literal which can be used to initialize an FName");

	/** Get the category name used for registering and unregistering */
	static constexpr auto GetCategoryName() { return GameplayDebuggerCategoryType::GetCategoryName(); }

	/** Bind this function to IGameplayDebugger::FOnGetCategory */
	FORCEINLINE static TSharedRef<FGameplayDebuggerCategory> MakeInstance()
	{
		// One additional indirection is required so this function can be easily bound to delegates.
		return MakeInstance_Internal();
	}

private:
	template <
		bool bEnable =
			TModels<GameplayDebuggerCategoryConcepts::CHasExplicitInstanceMethod, GameplayDebuggerCategoryType>::Value>
	FORCEINLINE static typename TEnableIf<bEnable, TSharedRef<FGameplayDebuggerCategory>>::Type MakeInstance_Internal()
	{
		return GameplayDebuggerCategoryType::MakeInstance();
	}

	template <
		bool bEnable =
			TModels<GameplayDebuggerCategoryConcepts::CHasExplicitInstanceMethod, GameplayDebuggerCategoryType>::Value
			== false>
	FORCEINLINE static typename TEnableIf<bEnable, TSharedRef<FGameplayDebuggerCategory>>::Type MakeInstance_Internal(
		int32 OverloadArg = 0)
	{
		return MakeShared<GameplayDebuggerCategoryType>();
	}
};

/**
 * Compile time list of FGameplayDebuggerCategory types to register and unregister them in bulk.
 * This base template is only used as variadic base and should not contain any "real" functionality.
 */
template <typename DebuggerCategoryType, typename... OtherTypes>
class TGameplayDebuggerCategoryTypeList
{
public:
	using SingleType = TGameplayDebuggerCategoryTypeList<DebuggerCategoryType>;
	using ExpandOtherTypes = TGameplayDebuggerCategoryTypeList<OtherTypes...>;

	TGameplayDebuggerCategoryTypeList()
	{
		static_assert(
			sizeof(DebuggerCategoryType) == -1,
			"Do not construct instances of TGameplayDebuggerCategoryTypeList! The class is meant to be used with "
			"static invocations only.");
	};

	template <EGameplayDebuggerCategoryState InitialState>
	static void RegisterCategories(IGameplayDebugger& GameplayDebugger)
	{
		SingleType::template RegisterCategories<InitialState>(GameplayDebugger);
		ExpandOtherTypes::template RegisterCategories<InitialState>(GameplayDebugger);
	}

	static void UnregisterCategories(IGameplayDebugger& GameplayDebugger)
	{
		SingleType::UnregisterCategories(GameplayDebugger);
		ExpandOtherTypes::UnregisterCategories(GameplayDebugger);
	}
};

/**
 * Compile time list of FGameplayDebuggerCategory types to register and unregister them in bulk.
 * This is the specialization for a single debugger type.
 * The functions in this specialization will be ultimately called for all types.
 */
template <typename DebuggerCategoryType>
class TGameplayDebuggerCategoryTypeList<DebuggerCategoryType>
{
public:
	static_assert(
		TIsDerivedFrom<DebuggerCategoryType, FGameplayDebuggerCategory>::Value,
		"GameplayDebuggerCategoryType must be derived from FGameplayDebuggerCategory");

	TGameplayDebuggerCategoryTypeList()
	{
		static_assert(
			sizeof(DebuggerCategoryType) == -1,
			"Do not construct instances of TGameplayDebuggerCategoryTypeList! The class is meant to be used with "
			"static invocations only.");
	};

	template <EGameplayDebuggerCategoryState InitialState>
	static void RegisterCategories(IGameplayDebugger& GameplayDebugger)
	{
		GameplayDebugger.RegisterCategory(
			TGameplayDebuggerCategoryTraits<DebuggerCategoryType>::GetCategoryName(),
			IGameplayDebugger::FOnGetCategory::CreateStatic(
				TGameplayDebuggerCategoryTraits<DebuggerCategoryType>::MakeInstance),
			InitialState);
	}

	static void UnregisterCategories(IGameplayDebugger& GameplayDebugger)
	{
		GameplayDebugger.UnregisterCategory(TGameplayDebuggerCategoryTraits<DebuggerCategoryType>::GetCategoryName());
	}
};

#endif
