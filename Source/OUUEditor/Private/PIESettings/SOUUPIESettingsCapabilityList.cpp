// Copyright (c) 2026 Jonas Reich & Contributors

#include "PIESettings/SOUUPIESettingsCapabilityList.h"

#include "PIESettings/OUUPIESettingsRegistry.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace OUU::Editor::Private::PIESettings
{
	using namespace OUU::Editor::PIESettings;

	// Width of the status column, wide enough for "Unavailable" so the capability names line up.
	static constexpr float GStatusColumnWidth = 90.f;

	//------------------------------------------------------------------------------------------------------------------
	void SPIESettingsCapabilityList::Construct(const FArguments& InArgs)
	{
		const TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);

		for (const FCapability& Capability : GetCapabilities())
		{
			if (Capability.Evaluate == nullptr)
			{
				continue;
			}

			auto GetState = [Evaluate = Capability.Evaluate]() { return Evaluate(); };

			// clang-format off
			Rows->AddSlot()
			.AutoHeight()
			.Padding(0.f, 2.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(GStatusColumnWidth)
					[
						SNew(STextBlock)
						.ColorAndOpacity_Lambda([GetState]() { return GetStatusColor(GetState().Status); })
						.Text_Lambda([GetState]() { return GetStatusDisplayName(GetState().Status); })
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(STextBlock)
					.Font(FAppStyle::Get().GetFontStyle("PropertyWindow.BoldFont"))
					.ToolTipText(Capability.Description)
					.Text(Capability.DisplayName)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SNew(STextBlock)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.AutoWrapText(true)
					.Text_Lambda([GetState]() { return GetState().Reason; })
				]
			];
			// clang-format on
		}

		ChildSlot[Rows];
	}
} // namespace OUU::Editor::Private::PIESettings
