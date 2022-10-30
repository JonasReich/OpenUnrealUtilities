// Copyright by Grimlore Games & THQ Nordic

#pragma once

#if WITH_GAMEPLAY_DEBUGGER

	#include "CoreMinimal.h"

	#include "GameplayDebuggerAddonBase.h"
	#include "GameplayDebuggerCategory.h"
	#include "GameplayDebuggerExtension.h"

namespace OUU::Runtime::Private
{
	template <class UserClass>
	using TOUUKeyBindFunctionType =
		typename FGameplayDebuggerInputHandler::FHandler::TRawMethodDelegate<UserClass>::FMethodPtr;

	/**
	 * Templated base class for gameplay debugger extensions to support advanced key bindings.
	 * Do not inherit from this directly! Instead, use the derived non-template types below.
	 */
	template <typename SuperType>
	class OUURUNTIME_API TOUUGameplayDebuggerAddonBase : public SuperType
	{
	public:
		using Super = SuperType;
		using SelfType = typename TOUUGameplayDebuggerAddonBase<Super>;

		static_assert(
			TOr<TIsSame<SuperType, FGameplayDebuggerAddonBase>,
				TIsSame<SuperType, FGameplayDebuggerCategory>,
				TIsSame<SuperType, FGameplayDebuggerExtension>>::Value,
			"SuperType must be derived from FGameplayDebuggerAddonBase (either FGameplayDebuggerCatgegory or "
			"FGameplayDebuggerExtension)");

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
			TOUUKeyBindFunctionType<UserClass> KeyHandlerFunc,
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

			return Super::BindKeyPress<SelfType>(
				FGameplayDebuggerInputHandlerConfig{ConfigName, DefaultKeyName, DefaultModifier},
				this,
				GetBoolSwitchKeyHandlerFunc(SwitchId),
				InputMode);
		}

		void PrintKeyBinds(FGameplayDebuggerCanvasContext& CanvasContext);

		FString GetKeyBindsString_SingleLine() const;

		void PrintKeyBind(FGameplayDebuggerCanvasContext& CanvasContext, int32 KeyBindIndex);

		FString GetKeyBindString(int32 KeyBindIndex) const;

		bool GetInputBoolSwitchValue(FName InputNameId) const;

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

		static TOUUKeyBindFunctionType<TOUUGameplayDebuggerAddonBase<Super>> GetBoolSwitchKeyHandlerFunc(
			int32 SwitchNum);
	};
} // namespace OUU::Runtime::Private

// Base class for custom addons that are neither categories nor extensions
class OUURUNTIME_API FOUUGameplayDebuggerAddonBase_Base :
	public OUU::Runtime::Private::TOUUGameplayDebuggerAddonBase<FGameplayDebuggerAddonBase>
{
};

// Base class for categories
class OUURUNTIME_API FOUUGameplayDebuggerCategory_Base :
	public OUU::Runtime::Private::TOUUGameplayDebuggerAddonBase<FGameplayDebuggerCategory>
{
	// Add a function like this to your child class to make it usable with TGameplayDebuggerCategoryTypeList
	// static auto GetCategoryName() { return TEXT("CategoryName"); }
};

// Base class for extensions
class OUURUNTIME_API FOUUGameplayDebuggerExtension_Base :
	public OUU::Runtime::Private::TOUUGameplayDebuggerAddonBase<FGameplayDebuggerExtension>
{
public:
	// - FGameplayDebuggerExtension
	virtual FString GetDescription() const override;
	// --
};

#endif
