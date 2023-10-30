// Copyright (c) 2023 Jonas Reich & Contributors

#include "Slate/SimpleClassPickerDialog.h"

#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "Kismet2/SClassPickerDialog.h"
#include "Modules/ModuleManager.h"

class FAssetClassParentFilter : public IClassViewerFilter
{
public:
	/** All children of these classes will be included unless filtered out by another setting. */
	TSet<const UClass*> AllowedChildrenOfClasses;

	/** Disallowed class flags. */
	EClassFlags DisallowedClassFlags;

	bool IsClassAllowed(
		const FClassViewerInitializationOptions& InInitOptions,
		const UClass* InClass,
		TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
	{
		return !InClass->HasAnyClassFlags(DisallowedClassFlags)
			&& InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InClass) != EFilterReturn::Failed;
	}

	bool IsUnloadedClassAllowed(
		const FClassViewerInitializationOptions& InInitOptions,
		const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData,
		TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
	{
		return !InUnloadedClassData->HasAnyClassFlags(DisallowedClassFlags)
			&& InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InUnloadedClassData)
			!= EFilterReturn::Failed;
	}
};

namespace OUU::Editor
{
	UClass* OpenSimpleClassPickerDialog(UClass* ParentClass, EClassFlags DisallowedClassFlags, const FText TitleText)
	{
		TSharedRef<FAssetClassParentFilter> Filter = MakeShareable(new FAssetClassParentFilter);
		Filter->DisallowedClassFlags = DisallowedClassFlags;
		Filter->AllowedChildrenOfClasses = {ParentClass};

		// Fill in options
		FClassViewerInitializationOptions Options;
		Options.Mode = EClassViewerMode::ClassPicker;
		Options.ClassFilters.Add(Filter);

		// Load the classviewer module to display a class picker
		FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

		UClass* NewChosenClass = nullptr;
		if (SClassPickerDialog::PickClass(TitleText, Options, OUT NewChosenClass, ParentClass))
		{
			return NewChosenClass;
		}

		return nullptr;
	}
} // namespace OUU::Editor
