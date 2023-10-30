// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "UObject/StrongObjectPtr.h"
#include "WorldPartition/WorldPartitionBuilder.h"
#include "WorldPartition/WorldPartitionMiniMap.h"

#include "ExportWorldPartitionMiniMapBuilder.generated.h"

/**
 * Export the minimap of the current world in Saved/MinimapExports/<MapName>.png
 */
UCLASS()
class OUUEDITOR_API UExportWorldPartitionMiniMapBuilder : public UWorldPartitionBuilder
{
	GENERATED_BODY()
public:
	// - UWorldPartitionBuilder
	bool RequiresCommandletRendering() const override { return true; }
	ELoadingMode GetLoadingMode() const override { return ELoadingMode::Custom; }

protected:
	bool PreRun(UWorld* World, FPackageSourceControlHelper& PackageHelper) override;
	bool RunInternal(UWorld* World, const FCellInfo& InCellInfo, FPackageSourceControlHelper& PackageHelper)
		override;
	bool PostRun(UWorld* World, FPackageSourceControlHelper& PackageHelper, const bool bInRunSuccess) override;
	// --

private:
	UPROPERTY()
	TObjectPtr<AWorldPartitionMiniMap> WorldMiniMap;
};
