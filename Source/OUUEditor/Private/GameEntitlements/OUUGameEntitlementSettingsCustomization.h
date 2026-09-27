// Copyright (c) 2026 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "IDetailCustomization.h"

namespace OUU::Editor::Private::GameEntitlements
{
	// Replaces the raw Entitlements array of UOUUGameEntitlementSettings with SGameEntitlementsGrid.
	class FGameEntitlementSettingsCustomization : public IDetailCustomization
	{
	public:
		// - IDetailCustomization
		void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	};
} // namespace OUU::Editor::Private::GameEntitlements
