// Copyright (c) 2021 Jonas Reich

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "SpiralIdUtilities.generated.h"

/**
 * Utility functions for converting coordinates to spiral IDs and vice versa.
 * Spiral IDs allow indexing a 2D grid with only positive numbers starting at a center location.
 * The resulting grid looks somewhat like this (notice flipped Y axis like in UE4 when looking down)
 *
 *    -2  -1    0   +1   +2
 * -2 +----+----+----+----+
 *    | 15 | 04 | 05 | 06 |
 * -1 +----+----+----+----+
 *    | 14 | 03 | 00 | 07 |
 *  0 +----+----o----+----+
 *    | 13 | 02 | 01 | 08 |
 * +1 +----+----+----+----+
 *    | 12 | 11 | 10 | 09 |
 * +2 +----+----+----+----+
 */
UCLASS(BlueprintType)
class OUURUNTIME_API USpiralIdUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Convert grid coordinates to spiral coordinates the same way it is calculated in Houdini
	 * Note that this uses grid coordinates as in grid cells, so the world coordinates will likely be different from X and Y.
	 */
	UFUNCTION(BlueprintPure)
	static int32 ConvertCoordinatesToSpiralId(const int32 X, const int32 Y);

	UFUNCTION(BlueprintPure)
	static int32 ConvertWorldLocation2DToSpiralId(const FVector2D& Location, float GridSize);

	UFUNCTION(BlueprintPure)
	static int32 ConvertWorldLocationToSpiralId(const FVector& Location, float GridSize);

	UFUNCTION(BlueprintPure)
	static FIntPoint ConvertSpiralIdToCoordinates(const int32 SpiralId);

	UFUNCTION(BlueprintPure)
	static FVector2D ConvertSpiralIdToCenterLocation(const int32 SpiralId, float GridSize);

	UFUNCTION(BlueprintPure)
	static FBox2D ConvertSpiralIdToBounds(int32 SpiralId, float GridSize);

	UFUNCTION(BlueprintPure)
	static FBox ConvertSpiralIdToBounds3D(int32 SpiralId, float GridSize, float BoundsHeight);
};
