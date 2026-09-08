// Copyright (c) 2026 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "PIESettings/OUUPIESettingsRegistry.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

namespace OUU::Editor::Private::PIESettings
{
	// Body of the PIE settings tab: capability statuses on top, the settings they depend on below.
	// Built once from the registry, so a module registering entries after the tab is open needs the tab reopened.
	class SPIESettingsPanel : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPIESettingsPanel)
			{
			}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);

	private:
		TSharedRef<SWidget> MakeSettingsGroup(const FString& Group, const FText& Heading);
	};
} // namespace OUU::Editor::Private::PIESettings
