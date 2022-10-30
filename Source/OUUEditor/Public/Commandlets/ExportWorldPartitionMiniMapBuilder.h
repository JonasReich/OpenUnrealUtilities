// Copyright by Grimlore Games & THQ Nordic

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
	virtual bool RequiresCommandletRendering() const override { return true; }
	virtual ELoadingMode GetLoadingMode() const override { return ELoadingMode::Custom; }
protected:
	virtual bool PreRun(UWorld* World, FPackageSourceControlHelper& PackageHelper) override;
	virtual bool RunInternal(UWorld* World, const FCellInfo& InCellInfo, FPackageSourceControlHelper& PackageHelper)
		override;
	virtual bool PostRun(UWorld* World, FPackageSourceControlHelper& PackageHelper, const bool bInRunSuccess) override;
	// --

private:
	TObjectPtr<AWorldPartitionMiniMap> WorldMiniMap;
};
