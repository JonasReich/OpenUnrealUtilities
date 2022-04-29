// Copyright (c) 2022 Jonas Reich

#include "OUUMapsToCookSettingsDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Modules/ModuleManager.h"
#include "OUUMapsToCookSettings.h"
#include "Widgets/Input/STextComboBox.h"

namespace OUU::Editor
{
	namespace Private::OUUMapsToCookSettingsDetails
	{
		TSharedRef<IDetailCustomization> MakeInstance() { return MakeShareable(new FOUUMapsToCookSettingsDetails()); }
	} // namespace Private::OUUMapsToCookSettingsDetails

	void FOUUMapsToCookSettingsDetails::Register()
	{
		FPropertyEditorModule& PropertyModule =
			FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomClassLayout(
			UOUUMapsToCookSettings::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&Private::OUUMapsToCookSettingsDetails::MakeInstance));
	}

	void FOUUMapsToCookSettingsDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
	{
		{
			const TSharedRef<IPropertyHandle> OptionsPropertyHandle =
				DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UOUUMapsToCookSettings, ConfigSections));

			auto ChangedDelegate =
				FSimpleDelegate::CreateSP(this, &FOUUMapsToCookSettingsDetails::UpdateConfigSectionNames);
			OptionsPropertyHandle->SetOnPropertyValueChanged(ChangedDelegate);
			OptionsPropertyHandle->SetOnChildPropertyValueChanged(ChangedDelegate);

			ConfigSectionNames_Property = OptionsPropertyHandle->AsArray();
		}

		DefaultConfigSection_Property =
			DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UOUUMapsToCookSettings, DefaultConfigSection));
		IDetailCategoryBuilder& TargetCategoryBuilder =
			DetailBuilder.EditCategory(FName(*DefaultConfigSection_Property->GetMetaData("Category")));
		DetailBuilder.HideProperty(DefaultConfigSection_Property);

		TargetCategoryBuilder.AddCustomRow(DefaultConfigSection_Property->GetPropertyDisplayName())
			.NameContent()[DefaultConfigSection_Property->CreatePropertyNameWidget()]
			.ValueContent()
			.MaxDesiredWidth(500.0f)
			.MinDesiredWidth(100.0f)
				[SNew(SHorizontalBox)
				 + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					   [SAssignNew(DefaultConfigSection_Combobox, STextComboBox)
							.Font(IDetailLayoutBuilder::GetDetailFont())
							.OptionsSource(&ConfigSectionNames)
							.OnSelectionChanged(this, &FOUUMapsToCookSettingsDetails::OnSelectedConfigSectionChanged)]];

		UpdateConfigSectionNames();
	}

	void FOUUMapsToCookSettingsDetails::UpdateConfigSectionNames()
	{
		ConfigSectionNames.Empty();
		// always add an empty element, so users can also select to clear the default config section name
		ConfigSectionNames.Add(MakeShared<FString>(""));
		uint32 NumArrayElements;
		ConfigSectionNames_Property->GetNumElements(NumArrayElements);
		for (uint32 i = 0; i < NumArrayElements; i++)
		{
			const auto ArrayElement = ConfigSectionNames_Property->GetElement(i);
			FString StringValue = "";
			ArrayElement->GetValue(StringValue);
			ConfigSectionNames.Add(MakeShared<FString>(StringValue));
		}

		FString CurrentValue;
		DefaultConfigSection_Property->GetValue(CurrentValue);
		if (CurrentValue.IsEmpty())
		{
			// Default to first element + save to settings when not set explicitly.
			CurrentSelectedIndex = 0;
			OnSelectedConfigSectionChanged(ConfigSectionNames[CurrentSelectedIndex], ESelectInfo::Direct);
		}
		else
		{
			for (int32 i = 0; i < ConfigSectionNames.Num(); i++)
			{
				if (*ConfigSectionNames[i] == CurrentValue)
				{
					CurrentSelectedIndex = i;
					break;
				}
			}
		}

		CurrentSelectedIndex = FMath::Clamp(CurrentSelectedIndex, 0, ConfigSectionNames.Num());
		DefaultConfigSection_Combobox->SetSelectedItem(ConfigSectionNames[CurrentSelectedIndex]);
	}

	void FOUUMapsToCookSettingsDetails::OnSelectedConfigSectionChanged(
		TSharedPtr<FString> NewValue,
		ESelectInfo::Type SelectInfo) const
	{
		DefaultConfigSection_Property->SetValue(*NewValue);
	}
} // namespace OUU::Editor
