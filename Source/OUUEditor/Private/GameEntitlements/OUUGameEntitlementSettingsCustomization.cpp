// Copyright (c) 2026 Jonas Reich & Contributors

#include "GameEntitlements/OUUGameEntitlementSettingsCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "GameEntitlements/OUUGameEntitlementsSettings.h"
#include "GameEntitlements/SOUUGameEntitlementsGrid.h"
#include "PropertyHandle.h"

namespace OUU::Editor::Private::GameEntitlements
{
	//------------------------------------------------------------------------------------------------------------------
	void FGameEntitlementSettingsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
	{
		const TSharedRef<IPropertyHandle> EntitlementsHandle =
			DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UOUUGameEntitlementSettings, Entitlements));

		TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
		DetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);
		auto* Settings =
			CustomizedObjects.Num() == 1 ? Cast<UOUUGameEntitlementSettings>(CustomizedObjects[0].Get()) : nullptr;
		if (Settings == nullptr)
		{
			return;
		}

		DetailBuilder.HideProperty(EntitlementsHandle);

		// Separate category, so the grid is listed below the regular settings.
		IDetailCategoryBuilder& GridCategory =
			DetailBuilder.EditCategory(TEXT("EntitlementsGrid"), INVTEXT("Entitlements"), ECategoryPriority::Uncommon);

		// clang-format off
		GridCategory.AddCustomRow(INVTEXT("Entitlements"))
			.WholeRowContent()
			[
				SNew(SGameEntitlementsGrid, Settings, EntitlementsHandle)
			];
		// clang-format on
	}
} // namespace OUU::Editor::Private::GameEntitlements
