// Copyright (c) 2026 Jonas Reich & Contributors

#include "PIESettings/OUUPIESettingsRegistry.h"

#include "HAL/IConsoleManager.h"
#include "ISettingsModule.h"
#include "ISinglePropertyView.h"
#include "Misc/ConfigCacheIni.h"
#include "Modules/ModuleManager.h"
#include "PIESettings/SOUUPIESettingsConsoleVariableRow.h"
#include "PropertyEditorModule.h"
#include "Styling/AppStyle.h"
#include "Widgets/Text/STextBlock.h"

namespace OUU::Editor::Private::PIESettings
{
	using namespace OUU::Editor::PIESettings;

	static TArray<FSettingEntry> GSettings;
	static TArray<FCapability> GCapabilities;

	//------------------------------------------------------------------------------------------------------------------
	static TSharedRef<SWidget> MakeErrorWidget(const FText& Message)
	{
		return SNew(STextBlock).ColorAndOpacity(FAppStyle::Get().GetSlateColor("Colors.AccentRed")).Text(Message);
	}

	//------------------------------------------------------------------------------------------------------------------
	static void SortSettings()
	{
		GSettings.StableSort([](const FSettingEntry& Lhs, const FSettingEntry& Rhs) {
			if (Lhs.Group != Rhs.Group)
			{
				return Lhs.Group < Rhs.Group;
			}
			return Lhs.SortOrder < Rhs.SortOrder;
		});
	}
} // namespace OUU::Editor::Private::PIESettings

namespace OUU::Editor::PIESettings
{
	using namespace OUU::Editor::Private::PIESettings;

	//------------------------------------------------------------------------------------------------------------------
	void OpenDeveloperSettings(const UDeveloperSettings* Settings)
	{
		if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
		{
			SettingsModule
				->ShowViewer(Settings->GetContainerName(), Settings->GetCategoryName(), Settings->GetSectionName());
		}
	}

	//------------------------------------------------------------------------------------------------------------------
	FSettingEntry MakePropertyEntry(const FString& Group, const FName& Id, UObject* Object, FName PropertyName)
	{
		FPropertyEditorModule& PropertyEditorModule =
			FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		FSettingEntry Entry;
		Entry.Group = Group;
		Entry.Id = Id;
		FProperty* Property = FindFProperty<FProperty>(Object->GetClass(), PropertyName, EFieldIterationFlags::Default);
		if (ensure(Property))
		{
			Entry.DisplayName = Property->GetDisplayNameText();
			Entry.Explanation = Property->GetToolTipText();

			if (CastField<FArrayProperty>(Property) && Object->IsA<UDeveloperSettings>())
			{
				Entry.MakeWidget =
					[&, WeakObject = TWeakObjectPtr<UObject>(Object), PropertyName]() -> TSharedRef<SWidget> {
					return SNew(SButton).Text(INVTEXT("Show in Settings")).OnClicked_Lambda([WeakObject]() {
						OpenDeveloperSettings(Cast<UDeveloperSettings>(WeakObject.Get()));
						return FReply::Handled();
					});
				};
			}
			else
			{
				Entry.MakeWidget =
					[&, WeakObject = TWeakObjectPtr<UObject>(Object), PropertyName]() -> TSharedRef<SWidget> {
					UObject* Target = WeakObject.Get();
					if (Target == nullptr)
					{
						return MakeErrorWidget(INVTEXT("Object is no longer valid"));
					}

					FSinglePropertyParams Params;
					Params.NamePlacement = EPropertyNamePlacement::Hidden;

					const TSharedPtr<ISinglePropertyView> View =
						PropertyEditorModule.CreateSingleProperty(Target, PropertyName, Params);
					if (View.IsValid() == false)
					{
						return MakeErrorWidget(FText::Format(
							INVTEXT("'{0}' is not a property of {1}"),
							FText::FromName(PropertyName),
							FText::FromString(Target->GetClass()->GetName())));
					}

					// The property editor writes the value onto the object but never persists it; only the settings
					// panels do that, and we bypass them.
					View->SetOnPropertyValueChanged(
						FSimpleDelegate::CreateWeakLambda(Target, [Target]() { Target->SaveConfig(); }));

					return View.ToSharedRef();
				};
			}
		}
		return Entry;
	}

	//------------------------------------------------------------------------------------------------------------------
	FSettingEntry MakeConsoleVariableEntry(
		const FString& Group,
		const FName& ConsoleVariableName,
		const FText& Explanation)
	{
		FSettingEntry Entry;
		Entry.Group = Group;
		Entry.Id = ConsoleVariableName;
		Entry.ConsoleVariableName = ConsoleVariableName;
		Entry.DisplayName = FText::FromName(ConsoleVariableName);
		Entry.Explanation = Explanation;
		Entry.MakeWidget = [ConsoleVariableName]() -> TSharedRef<SWidget> {
			return SNew(SPIESettingsConsoleVariableRow, ConsoleVariableName);
		};
		return Entry;
	}

	//------------------------------------------------------------------------------------------------------------------
	FSettingEntry MakeWidgetEntry(const FString& Group, const FName& Id, TFunction<TSharedRef<SWidget>()> WidgetFactory)
	{
		FSettingEntry Entry;
		Entry.Group = Group;
		Entry.Id = Id;
		Entry.MakeWidget = MoveTemp(WidgetFactory);
		return Entry;
	}

	//------------------------------------------------------------------------------------------------------------------
	void RegisterSetting(FSettingEntry Entry)
	{
		if (ensureMsgf(Entry.Id.IsNone() == false, TEXT("PIE setting entries need an id to be unregistered by")))
		{
			UnregisterSetting(Entry.Id);
			GSettings.Add(MoveTemp(Entry));
			SortSettings();
		}
	}

	//------------------------------------------------------------------------------------------------------------------
	void UnregisterSetting(FName Id)
	{
		GSettings.RemoveAll([Id](const FSettingEntry& Entry) { return Entry.Id == Id; });
	}

	//------------------------------------------------------------------------------------------------------------------
	void RegisterCapability(FCapability Capability)
	{
		if (ensureMsgf(Capability.Id.IsNone() == false, TEXT("PIE capabilities need an id to be unregistered by")))
		{
			UnregisterCapability(Capability.Id);
			GCapabilities.Add(MoveTemp(Capability));
		}
	}

	//------------------------------------------------------------------------------------------------------------------
	void UnregisterCapability(FName Id)
	{
		GCapabilities.RemoveAll([Id](const FCapability& Capability) { return Capability.Id == Id; });
	}

	//------------------------------------------------------------------------------------------------------------------
	const TArray<FSettingEntry>& GetSettings() { return GSettings; }

	//------------------------------------------------------------------------------------------------------------------
	const TArray<FCapability>& GetCapabilities() { return GCapabilities; }

	//------------------------------------------------------------------------------------------------------------------
	ECapabilityStatus GetWorstCapabilityStatus()
	{
		ECapabilityStatus Worst = ECapabilityStatus::Available;
		for (const FCapability& Capability : GCapabilities)
		{
			if (Capability.Evaluate)
			{
				Worst = FMath::Max(Worst, Capability.Evaluate().Status);
			}
		}
		return Worst;
	}

	//------------------------------------------------------------------------------------------------------------------
	int32 SaveConsoleVariablesToUserConfig()
	{
		if (GConfig == nullptr)
		{
			return 0;
		}

		int32 SavedCount = 0;
		for (const FSettingEntry& Entry : GSettings)
		{
			if (Entry.ConsoleVariableName.IsNone())
			{
				continue;
			}

			const FString VariableName = Entry.ConsoleVariableName.ToString();
			const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(*VariableName);
			if (Variable == nullptr)
			{
				continue;
			}

			GConfig->SetString(TEXT("ConsoleVariables"), *VariableName, *Variable->GetString(), GEngineIni);
			++SavedCount;
		}

		if (SavedCount > 0)
		{
			GConfig->Flush(false, GEngineIni);
		}
		return SavedCount;
	}

	//------------------------------------------------------------------------------------------------------------------
	FText GetStatusDisplayName(ECapabilityStatus Status)
	{
		switch (Status)
		{
		case ECapabilityStatus::Available: return INVTEXT("Available");
		case ECapabilityStatus::Limited: return INVTEXT("Limited");
		case ECapabilityStatus::Unavailable: return INVTEXT("Unavailable");
		}
		return FText::GetEmpty();
	}

	//------------------------------------------------------------------------------------------------------------------
	FSlateColor GetStatusColor(ECapabilityStatus Status)
	{
		switch (Status)
		{
		case ECapabilityStatus::Available: return FAppStyle::Get().GetSlateColor("Colors.AccentGreen");
		case ECapabilityStatus::Limited: return FAppStyle::Get().GetSlateColor("Colors.AccentYellow");
		case ECapabilityStatus::Unavailable: return FAppStyle::Get().GetSlateColor("Colors.AccentRed");
		}
		return FSlateColor::UseForeground();
	}
} // namespace OUU::Editor::PIESettings
