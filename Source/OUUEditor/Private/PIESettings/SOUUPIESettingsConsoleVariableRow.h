// Copyright (c) 2026 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class IConsoleVariable;

namespace OUU::Editor::Private::PIESettings
{
	// Editor for a single console variable, reading the live value on every paint so a change made from the console or
	// from code is reflected without a refresh.
	class SPIESettingsConsoleVariableRow : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPIESettingsConsoleVariableRow)
			{
			}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, FName InConsoleVariableName);

	private:
		TSharedRef<SWidget> MakeValueWidget();
		TSharedRef<SWidget> MakeBoolWidget();
		TSharedRef<SWidget> MakeIntWidget();
		TSharedRef<SWidget> MakeFloatWidget();
		TSharedRef<SWidget> MakeStringWidget();

		FName ConsoleVariableName;

		// Null when the name does not resolve. Console variables are registered by static initialization and never
		// removed, so a resolved pointer stays valid for the editor session.
		IConsoleVariable* ConsoleVariable = nullptr;
	};
} // namespace OUU::Editor::Private::PIESettings
