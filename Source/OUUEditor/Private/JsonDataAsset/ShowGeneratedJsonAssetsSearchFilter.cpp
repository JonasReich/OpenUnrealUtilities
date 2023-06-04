// Copyright (c) 2023 Jonas Reich & Contributors

#include "JsonDataAsset/ShowGeneratedJsonAssetsSearchFilter.h"

#include "AssetRegistry/IAssetRegistry.h"
#include "FrontendFilters.h"
#include "JsonDataAsset/JsonDataAsset.h"
#include "LogOpenUnrealUtilities.h"

// Modeled after FFrontendFilter_ShowRedirectors
class FFrontendFilter_ShowGeneratedJsonAssets : public FFrontendFilter
{
public:
	FFrontendFilter_ShowGeneratedJsonAssets(TSharedPtr<FFrontendFilterCategory> InCategory) :
		FFrontendFilter(InCategory)
	{
		bAreJsonAssetsInBaseFilter = false;
	}

	static FTopLevelAssetPath TryConvertShortTypeNameToPathName(FName ClassName)
	{
		FTopLevelAssetPath ClassPathName;
		if (ClassName != NAME_None)
		{
			FString ShortClassName = ClassName.ToString();
			ClassPathName = UClass::TryConvertShortTypeNameToPathName<UStruct>(
				*ShortClassName,
				ELogVerbosity::Warning,
				TEXT("FFrontendFilter_ShowGeneratedJsonAssets using deprecated function"));
			UE_CLOG(
				ClassPathName.IsNull(),
				LogOpenUnrealUtilities,
				Error,
				TEXT("Failed to convert short class name %s to class path name."),
				*ShortClassName);
		}
		return ClassPathName;
	}

	void RefreshJsonClassPaths() const
	{
		// Only update the class paths once per frame.
		if (ClassPathUpdateFrameCounter == GFrameCounter)
			return;

		ClassPathUpdateFrameCounter = GFrameCounter;

		IAssetRegistry::Get()->GetDerivedClassNames(
			TArray<FTopLevelAssetPath>{FTopLevelAssetPath(TEXT("/Script/OUURuntime"), TEXT("JsonDataAsset"))},
			TSet<FTopLevelAssetPath>{},
			JsonDataAssetClassPaths);
	}

	// - FFrontendFilter
	virtual FString GetName() const override { return TEXT("ShowGeneratedJsonAssets"); }
	virtual FText GetDisplayName() const override { return INVTEXT("Show Generated Json Data Assets"); }
	virtual FText GetToolTipText() const override { return INVTEXT("Allow display of generated Json Data Assets."); }
	virtual bool IsInverseFilter() const override { return true; }
	virtual void SetCurrentFilter(TArrayView<const FName> InSourcePaths, const FContentBrowserDataFilter& InBaseFilter)
		override
	{
		const FContentBrowserDataClassFilter* ClassFilter =
			InBaseFilter.ExtraFilters.FindFilter<FContentBrowserDataClassFilter>();

		bAreJsonAssetsInBaseFilter = false;

		if (ClassFilter)
		{
			for (auto& Entry : ClassFilter->ClassNamesToInclude)
			{
				FTopLevelAssetPath EntryAsPath = TryConvertShortTypeNameToPathName(*Entry);
				if (JsonDataAssetClassPaths.Contains(EntryAsPath))
				{
					bAreJsonAssetsInBaseFilter = true;
					break;
				}
			}
		}
	}
	// --

	// - IFilter
	virtual bool PassesFilter(FAssetFilterType InItem) const override
	{
		RefreshJsonClassPaths();

		// Never hide json data assets if they are explicitly searched for
		if (!bAreJsonAssetsInBaseFilter)
		{
			const FContentBrowserItemDataAttributeValue ClassValue = InItem.GetItemAttribute(NAME_Class);
			auto ClassName = ClassValue.GetValue<FString>();
			FTopLevelAssetPath ClassPath = TryConvertShortTypeNameToPathName(*ClassName);
			if (ClassValue.IsValid() == false || JsonDataAssetClassPaths.Contains(ClassPath))
			{
				return false;
			}
		}

		return true;
	}
	// --

private:
	bool bAreJsonAssetsInBaseFilter;
	mutable TSet<FTopLevelAssetPath> JsonDataAssetClassPaths;
	mutable uint64 ClassPathUpdateFrameCounter = 0;
};

void UShowGeneratedJsonAssetsSearchFilter::AddFrontEndFilterExtensions(
	TSharedPtr<class FFrontendFilterCategory> DefaultCategory,
	TArray<TSharedRef<class FFrontendFilter>>& InOutFilterList) const
{
	/*
	#TODO-OUU Only add this filter if we actually imlemented all move/rename actions for json data assets.
	InOutFilterList.Add(MakeShareable(new FFrontendFilter_ShowGeneratedJsonAssets(DefaultCategory)));
	*/
}
