// Copyright (c) 2026 Jonas Reich & Contributors

#include "PIESettings/SOUUPIESettingsPanel.h"

#include "Framework/Notifications/NotificationManager.h"
#include "PIESettings/SOUUPIESettingsCapabilityList.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace OUU::Editor::Private::PIESettings
{
	using namespace OUU::Editor::PIESettings;

	static constexpr float GEditorColumnWidth = 260.f;

	//------------------------------------------------------------------------------------------------------------------
	void SPIESettingsPanel::Construct(const FArguments& InArgs)
	{
		TSharedPtr<SVerticalBox> VerticalBox;

		// clang-format off
		ChildSlot
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			.Padding(8.f)
			[
				SAssignNew(VerticalBox, SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.AutoWrapText(true)
					.Text(INVTEXT("This describes the configuration your next Play session will start with. Changes "
								  "made here do not affect a session that is already running."))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 4.f)
				[
					SNew(STextBlock)
					.Font(FAppStyle::Get().GetFontStyle("PropertyWindow.BoldFont"))
					.Text(INVTEXT("What you can test"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
					.Padding(6.f)
					[
						SNew(SPIESettingsCapabilityList)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 12.f)
				[
					SNew(SSeparator)
				]
			]
		];
		// clang-format on

		// #TODO change to explicit group declarations
		TSet<FString> Groups;
		for (auto& Entry : GetSettings())
		{
			Groups.Add(Entry.Group);
		}
		for (auto& Group : Groups)
		{
			VerticalBox->AddSlot().AutoHeight()[MakeSettingsGroup(Group, FText::FromString(Group))];
		}
	}

	//------------------------------------------------------------------------------------------------------------------
	TSharedRef<SWidget> SPIESettingsPanel::MakeSettingsGroup(const FString& Group, const FText& Heading)
	{
		const TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);

		for (const FSettingEntry& Entry : GetSettings())
		{
			if (Entry.Group != Group || Entry.MakeWidget == nullptr)
			{
				continue;
			}

			// clang-format off
			Rows->AddSlot()
			.AutoHeight()
			.Padding(0.f, 3.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(Entry.DisplayName)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						.AutoWrapText(true)
						.Text(Entry.Explanation)
					]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
					.WidthOverride(GEditorColumnWidth)
					[
						Entry.MakeWidget()
					]
				]
			];
			// clang-format on
		}

		const TSharedRef<SVerticalBox> GroupWidget = SNew(SVerticalBox);

		const TSharedRef<SHorizontalBox> HeadingRow = SNew(SHorizontalBox);

		// clang-format off
		HeadingRow->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Font(FAppStyle::Get().GetFontStyle("PropertyWindow.BoldFont"))
			.Text(Heading)
		];
		
		GroupWidget->AddSlot()
		.AutoHeight()
		.Padding(0.f, 0.f, 0.f, 4.f)
		[
			HeadingRow
		];

		GroupWidget->AddSlot()
		.AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.Padding(6.f)
			[
				Rows
			]
		];
		// clang-format on

		return GroupWidget;
	}
} // namespace OUU::Editor::Private::PIESettings
