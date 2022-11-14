
#include "GameplayDebugger/OUUGameplayDebuggerAddonBase.h"

#if WITH_GAMEPLAY_DEBUGGER

	#include "GameplayDebugger/GameplayDebuggerUtils.h"

namespace OUU::Runtime::Private
{
	template <typename SuperType>
	void TOUUGameplayDebuggerAddonBase<SuperType>::PrintKeyBinds(FGameplayDebuggerCanvasContext& CanvasContext)
	{
		CanvasContext
			.Printf(TEXT("{yellow}Key Bindings: %s"), *FString(InputNames.Num() > 0 ? TEXT("") : TEXT("None")));
		for (int32 i = 0; i < InputNames.Num(); i++)
		{
			PrintKeyBind(CanvasContext, i);
		}
		CanvasContext.MoveToNewLine();
	}

	template <typename SuperType>
	FString TOUUGameplayDebuggerAddonBase<SuperType>::GetKeyBindsString_SingleLine() const
	{
		TArray<FString> KeyBindStrings;
		for (int32 i = 0; i < InputNames.Num(); i++)
		{
			KeyBindStrings.Add(GetKeyBindString(i));
		}
		return FString::Join(KeyBindStrings, TEXT(" | "));
	}

	template <typename SuperType>
	void TOUUGameplayDebuggerAddonBase<SuperType>::PrintKeyBind(
		FGameplayDebuggerCanvasContext& CanvasContext,
		int32 KeyBindIndex)
	{
		if (!ensure(InputNames.IsValidIndex(KeyBindIndex)))
			return;
		CanvasContext.Print(GetKeyBindString(KeyBindIndex));
	}

	template <typename SuperType>
	FString TOUUGameplayDebuggerAddonBase<SuperType>::GetKeyBindString(int32 KeyBindIndex) const
	{
		if (!ensure(InputNames.IsValidIndex(KeyBindIndex)))
			return TEXT("");

		const FName& InputName = InputNames[KeyBindIndex];
		const FString BoolSwitchStatus = BoolSwitchIndices.Contains(InputName)
			?
			// case 1: we have a bool switch and print its current value
			FString::Printf(
				TEXT(" (%s{white})"),
				*OUU::Runtime::GameplayDebuggerUtils::GetColoredBoolString(GetInputBoolSwitchValue(InputName)))
			// case 2: we don't have a bool switch and print nothing
			: "";

		return FString::Printf(
			TEXT("{white}[{yellow}%s{white}]: %s%s"),
			*FGameplayDebuggerAddonBase::GetInputHandlerDescription(KeyBindIndex),
			*InputName.ToString(),
			*BoolSwitchStatus);
	}

	template <typename SuperType>
	bool TOUUGameplayDebuggerAddonBase<SuperType>::GetInputBoolSwitchValue(FName InputNameId) const
	{
		const int32 BoolIdx = BoolSwitchIndices[InputNameId];
		checkf(BoolIdx != INDEX_NONE, TEXT("Bool switches are never cleared, so should never crash!"));
		return BoolSwitchValues[BoolIdx];
	}

	template <typename SuperType>
	void TOUUGameplayDebuggerAddonBase<SuperType>::RegisterKeyBind(
		FName ConfigName,
		bool bBindAsBoolSwitch,
		bool bBoolSwitchDefaultValue)
	{
		InputNames.Add(ConfigName);

		if (bBindAsBoolSwitch)
		{
			const int32 BoolSwitchIndex = BoolSwitchValues.Add(bBoolSwitchDefaultValue);
			BoolSwitchIndices.Add(ConfigName, BoolSwitchIndex);
		}
	}

	template <typename SuperType>
	TOUUKeyBindFunctionType<TOUUGameplayDebuggerAddonBase<SuperType>> TOUUGameplayDebuggerAddonBase<
		SuperType>::GetBoolSwitchKeyHandlerFunc(int32 SwitchNum)
	{
		switch (SwitchNum)
		{
	#define BOOL_SWITCH_CASE(Num)                                                                                      \
	case Num: return &TOUUGameplayDebuggerAddonBase<SuperType>::HelperFunction_ToggleSwitch<Num>
			BOOL_SWITCH_CASE(0);
			BOOL_SWITCH_CASE(1);
			BOOL_SWITCH_CASE(2);
			BOOL_SWITCH_CASE(3);
			BOOL_SWITCH_CASE(4);
			BOOL_SWITCH_CASE(5);
			BOOL_SWITCH_CASE(6);
			BOOL_SWITCH_CASE(7);
			BOOL_SWITCH_CASE(8);
			BOOL_SWITCH_CASE(9);
			BOOL_SWITCH_CASE(10);
			BOOL_SWITCH_CASE(11);
			BOOL_SWITCH_CASE(12);
			BOOL_SWITCH_CASE(13);
			BOOL_SWITCH_CASE(14);
			BOOL_SWITCH_CASE(15);
			BOOL_SWITCH_CASE(16);
	#undef BOOL_SWITCH_CASE
		default:
			ensureMsgf(
				false,
				TEXT("More bool switches used than expected. Please add more cases to this function to "
					 "compensate for this!"));
			return nullptr;
		}
	}

	// Explicitly instantiate tempalated addon for gampelay debugger category other known super types
	template class TOUUGameplayDebuggerAddonBase<FGameplayDebuggerAddonBase>;
	template class TOUUGameplayDebuggerAddonBase<FGameplayDebuggerCategory>;
	template class TOUUGameplayDebuggerAddonBase<FGameplayDebuggerExtension>;

} // namespace OUU::Runtime::Private

FString FOUUGameplayDebuggerExtension_Base::GetDescription() const
{
	return GetKeyBindsString_SingleLine();
}

#endif