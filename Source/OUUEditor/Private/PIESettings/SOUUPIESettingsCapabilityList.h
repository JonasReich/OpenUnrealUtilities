// Copyright (c) 2026 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

namespace OUU::Editor::Private::PIESettings
{
	// One row per registered capability, re-evaluated on every paint. The predicates only read console variables and
	// config, so there is nothing to cache and nothing to refresh.
	class SPIESettingsCapabilityList : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPIESettingsCapabilityList)
			{
			}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
	};
} // namespace OUU::Editor::Private::PIESettings
