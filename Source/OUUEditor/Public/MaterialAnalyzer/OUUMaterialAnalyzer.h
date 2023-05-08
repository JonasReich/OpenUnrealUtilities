// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

struct FSplitterColumnSizeData;
class SWidget;

namespace OUU::Editor::MaterialAnalyzer
{
	void RegisterNomadTabSpawner();
	void UnregisterNomadTabSpawner();
} // namespace OUU::Editor::MaterialAnalyzer

namespace OUU::Editor::Private::MaterialAnalyzer
{
	enum class ESource
	{
		Undefined,
		Material,
		MaterialFunction
	};

	namespace Widgets
	{
		TSharedRef<SWidget> Splitter(
			TSharedPtr<FSplitterColumnSizeData> ColumnSizeData,
			TSharedRef<SWidget> LeftWidget,
			TSharedRef<SWidget> RightWidget);
	}
} // namespace OUU::Editor::Private::MaterialAnalyzer
