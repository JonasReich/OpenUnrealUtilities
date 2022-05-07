// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "IDetailCustomization.h"
#include "Types/SlateEnums.h"

class STextComboBox;
class IPropertyHandleArray;
class IPropertyHandle;

namespace OUU::Editor
{
	class FOUUMapsToCookSettingsDetails : public IDetailCustomization
	{
	public:
		static void Register();
		static void Unregister();

		// IDetailCustomization interface
		virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
		// --

	private:
		int32 CurrentSelectedIndex = 0;
		TArray<TSharedPtr<FString>> ConfigSectionNames;
		TSharedPtr<IPropertyHandleArray> ConfigSectionNames_Property;
		TSharedPtr<STextComboBox> DefaultConfigSection_Combobox;
		TSharedPtr<IPropertyHandle> DefaultConfigSection_Property;

		void UpdateConfigSectionNames();
		void OnSelectedConfigSectionChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo) const;
	};
} // namespace OUU::Editor
