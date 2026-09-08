// Copyright (c) 2026 Jonas Reich & Contributors

#include "PIESettings/SOUUPIESettingsConsoleVariableRow.h"

#include "HAL/IConsoleManager.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Text/STextBlock.h"

namespace OUU::Editor::Private::PIESettings
{
	// The editor UI stands in for the developer typing the command, so writes take the same priority the console does.
	static constexpr EConsoleVariableFlags GSetBy = ECVF_SetByConsole;

	//------------------------------------------------------------------------------------------------------------------
	void SPIESettingsConsoleVariableRow::Construct(const FArguments& InArgs, FName InConsoleVariableName)
	{
		ConsoleVariableName = InConsoleVariableName;
		ConsoleVariable = IConsoleManager::Get().FindConsoleVariable(*ConsoleVariableName.ToString());

		ChildSlot[MakeValueWidget()];
	}

	//------------------------------------------------------------------------------------------------------------------
	TSharedRef<SWidget> SPIESettingsConsoleVariableRow::MakeValueWidget()
	{
		if (ConsoleVariable == nullptr)
		{
			return SNew(STextBlock)
				.ColorAndOpacity(FAppStyle::Get().GetSlateColor("Colors.AccentRed"))
				.Text(FText::Format(INVTEXT("'{0}' does not exist"), FText::FromName(ConsoleVariableName)));
		}

		if (ConsoleVariable->IsVariableBool())
		{
			return MakeBoolWidget();
		}
		if (ConsoleVariable->IsVariableInt())
		{
			return MakeIntWidget();
		}
		if (ConsoleVariable->IsVariableFloat())
		{
			return MakeFloatWidget();
		}
		return MakeStringWidget();
	}

	//------------------------------------------------------------------------------------------------------------------
	TSharedRef<SWidget> SPIESettingsConsoleVariableRow::MakeBoolWidget()
	{
		return SNew(SCheckBox)
			.IsChecked_Lambda([this]() -> ECheckBoxState {
				return ConsoleVariable->GetBool() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda(
				[this](ECheckBoxState NewState) { ConsoleVariable->Set(NewState == ECheckBoxState::Checked, GSetBy); });
	}

	//------------------------------------------------------------------------------------------------------------------
	TSharedRef<SWidget> SPIESettingsConsoleVariableRow::MakeIntWidget()
	{
		return SNew(SNumericEntryBox<int32>)
			.AllowSpin(false)
			.Value_Lambda([this]() { return TOptional<int32>(ConsoleVariable->GetInt()); })
			.OnValueCommitted_Lambda(
				[this](int32 NewValue, ETextCommit::Type) { ConsoleVariable->Set(NewValue, GSetBy); });
	}

	//------------------------------------------------------------------------------------------------------------------
	TSharedRef<SWidget> SPIESettingsConsoleVariableRow::MakeFloatWidget()
	{
		return SNew(SNumericEntryBox<float>)
			.AllowSpin(false)
			.Value_Lambda([this]() { return TOptional<float>(ConsoleVariable->GetFloat()); })
			.OnValueCommitted_Lambda(
				[this](float NewValue, ETextCommit::Type) { ConsoleVariable->Set(NewValue, GSetBy); });
	}

	//------------------------------------------------------------------------------------------------------------------
	TSharedRef<SWidget> SPIESettingsConsoleVariableRow::MakeStringWidget()
	{
		return SNew(SEditableTextBox)
			.Text_Lambda([this]() { return FText::FromString(ConsoleVariable->GetString()); })
			.OnTextCommitted_Lambda(
				[this](const FText& NewText, ETextCommit::Type) { ConsoleVariable->Set(*NewText.ToString(), GSetBy); });
	}
} // namespace OUU::Editor::Private::PIESettings
