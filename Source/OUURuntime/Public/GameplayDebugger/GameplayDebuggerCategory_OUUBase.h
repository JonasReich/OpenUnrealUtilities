// Copyright (c) 2022 Jonas Reich

#pragma once

#if WITH_GAMEPLAY_DEBUGGER
	#include "CoreMinimal.h"
	#include "GameplayDebuggerCategory.h"

/**
 * Base class for gameplay debugger categories with some additional utility functionality.
 */
class OUURUNTIME_API FGameplayDebuggerCategory_OUUBase : public FGameplayDebuggerCategory
{
public:
	using Super = FGameplayDebuggerCategory;

	template <class UserClass>
	using TKeyBindFunctionType =
		typename FGameplayDebuggerInputHandler::FHandler::TRawMethodDelegate<UserClass>::FMethodPtr;

	// Add a function like this to your child class to make it usable with TGameplayDebuggerCategoryTypeList
	// static auto GetCategoryName() { return TEXT("OUU Base (abstract)"); }

	/**
	 * creates new key binding handler: customizable key press, stored in config files.
	 * Use this version instead of Super::BindKeyPress to make sure all inputs can be printed using PrintKeyBinds()
	 */
	template <class UserClass>
	bool BindKeyPress(
		const FName ConfigName,
		const FName DefaultKeyName,
		const FGameplayDebuggerInputModifier& DefaultModifier,
		UserClass* KeyHandlerObject,
		TKeyBindFunctionType<UserClass> KeyHandlerFunc,
		EGameplayDebuggerInputMode InputMode = EGameplayDebuggerInputMode::Local)
	{
		RegisterKeyBind(ConfigName, false, false);

		return Super::BindKeyPress<UserClass>(
			FGameplayDebuggerInputHandlerConfig{ConfigName, DefaultKeyName, DefaultModifier},
			KeyHandlerObject,
			KeyHandlerFunc,
			InputMode);
	}

	bool BindKeyPress_Switch(
		const FName ConfigName,
		const FName DefaultKeyName,
		const FGameplayDebuggerInputModifier& DefaultModifier,
		EGameplayDebuggerInputMode InputMode = EGameplayDebuggerInputMode::Local,
		bool bBoolSwitchDefaultValue = false)
	{
		int32 SwitchId = BoolSwitchValues.Num();
		RegisterKeyBind(ConfigName, true, bBoolSwitchDefaultValue);

		return Super::BindKeyPress<FGameplayDebuggerCategory_OUUBase>(
			FGameplayDebuggerInputHandlerConfig{ConfigName, DefaultKeyName, DefaultModifier},
			this,
			GetBoolSwitchKeyHandlerFunc(SwitchId),
			InputMode);
	}

	void PrintKeyBinds(FGameplayDebuggerCanvasContext& CanvasContext);

	void PrintKeyBind(FGameplayDebuggerCanvasContext& CanvasContext, int32 KeyBindIndex);

	bool GetInputBoolSwitchValue(FName InputNameId);

protected:
	TArray<FName> InputNames;
	TMap<FName, int32> BoolSwitchIndices;
	TArray<bool> BoolSwitchValues;

private:
	void RegisterKeyBind(FName ConfigName, bool bBindAsBoolSwitch, bool bBoolSwitchDefaultValue);

	template <int32 Index>
	void HelperFunction_ToggleSwitch()
	{
		BoolSwitchValues[Index] = !BoolSwitchValues[Index];
	}

	static TKeyBindFunctionType<FGameplayDebuggerCategory_OUUBase> GetBoolSwitchKeyHandlerFunc(int32 SwitchNum);
};

#endif
