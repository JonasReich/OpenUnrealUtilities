// Copyright (c) 2022 Jonas Reich

#include "GameplayDebugger/GameplayDebuggerCategory_OUUBase.h"

#include "GameplayDebugger/GameplayDebuggerUtils.h"

#if WITH_GAMEPLAY_DEBUGGER

void FGameplayDebuggerCategory_OUUBase::PrintKeyBinds(FGameplayDebuggerCanvasContext& CanvasContext)
{
	CanvasContext.Printf(TEXT("{yellow}Key Bindings: %s"), *FString(InputNames.Num() > 0 ? TEXT("") : TEXT("None")));
	for (int32 i = 0; i < InputNames.Num(); i++)
	{
		PrintKeyBind(CanvasContext, i);
	}
	CanvasContext.MoveToNewLine();
}

void FGameplayDebuggerCategory_OUUBase::PrintKeyBind(FGameplayDebuggerCanvasContext& CanvasContext, int32 KeyBindIndex)
{
	if (!ensure(InputNames.IsValidIndex(KeyBindIndex)))
		return;

	const FName& InputName = InputNames[KeyBindIndex];
	const FString BoolSwitchStatus = BoolSwitchIndices.Contains(InputName)
		?
		// case 1: we have a bool switch and print its current value
		FString::Printf(
			TEXT(" (%s{white})"),
			*OUU::Runtime::GameplayDebuggerUtils::GetColoredBoolString(GetInputBoolSwitchValue(InputName)))
		// case 2: we don't have a bool switch and print nothing
		: "";

	CanvasContext.Printf(
		TEXT("{white}[{yellow}%s{white}]: %s%s"),
		*GetInputHandlerDescription(KeyBindIndex),
		*InputName.ToString(),
		*BoolSwitchStatus);
}

bool FGameplayDebuggerCategory_OUUBase::GetInputBoolSwitchValue(FName InputNameId)
{
	const int32 BoolIdx = BoolSwitchIndices[InputNameId];
	checkf(BoolIdx != INDEX_NONE, TEXT("Bool switches are never cleared, so should never crash!"));
	return BoolSwitchValues[BoolIdx];
}

void FGameplayDebuggerCategory_OUUBase::RegisterKeyBind(
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

FGameplayDebuggerCategory_OUUBase::TKeyBindFunctionType<FGameplayDebuggerCategory_OUUBase>
	FGameplayDebuggerCategory_OUUBase::GetBoolSwitchKeyHandlerFunc(int32 SwitchNum)
{
	switch (SwitchNum)
	{
	#define BOOL_SWITCH_CASE(Num)                                                                                      \
	case Num: return &HelperFunction_ToggleSwitch<Num>
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

#endif
