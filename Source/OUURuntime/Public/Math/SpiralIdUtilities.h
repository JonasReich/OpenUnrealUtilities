// Copyright (c) 2022 Jonas Reich

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "SpiralIdUtilities.generated.h"

/**
 * The type of coordinate system the spiral should use
 * By using LeftHanded in a right handed coordinate system, you can change the winding direction
 * from counter clockwise to clockwise (and vice versa)
 * Because UE4 uses a left handed coordinate system you will have to use LeftHanded when using the utilities below with
 * UE4 coordinates
 */
UENUM(BlueprintType)
enum class ESpiralCoordinateSystemType : uint8
{
	RightHanded,
	LeftHanded
};

/**
 * Utility functions for converting traditional XY coordinates to counter clockwise spiral IDs and vice versa.
 * Spiral IDs allow indexing a 2D grid with only positive numbers starting at the origin.
 * The grid/column indices are not in UE4 coordinate space but based right handed coordinate system
 * like it is found in Unity3D or Houdini.
 * The resulting grid looks like this:
 *
 *      -2   -1   +0   +1
 *    +----+----+----+----+
 * +2 | 12 | 11 | 10 | 09 |
 *    +----+----+----+----+
 * +1 | 13 | 02 | 01 | 08 |
 *    +----+----o----+----+
 * +0 | 14 | 03 | 00 | 07 |
 *    +----+----+----+----+
 * -1 | 15 | 04 | 05 | 06 |
 *    +----+----+----+----+
 *
 * Note that the spiral origin is somewhere between the 0 and +1 row/columns.
 * You will rarely need the row column indices themselves, but usually want to convert spatial UE4 coordinates.
 * Remember that UE4 uses a left handed coordinate system when soing so.
 *
 *   -2   -1    0   +1   +2
 * +2 +----+----+----+----+
 *    | 12 | 11 | 10 | 09 |
 * +1 +----+----+----+----+
 *    | 13 | 02 | 01 | 08 |
 *  0 +----+----o----+----+
 *    | 14 | 03 | 00 | 07 |
 * -1 +----+----+----+----+
 *    | 15 | 04 | 05 | 06 |
 * -2 +----+----+----+----+
 *
 * The grid size can of course also be different than 1, then we get slightly different numbers,
 * e.g. for a grid width of 150 units you would get a grid like this:
 *
 *    -300 -150   0  +150 +300
 * +300 +----+----+----+----+
 *      | 12 | 11 | 10 | 09 |
 * +150 +----+----+----+----+
 *      | 13 | 02 | 01 | 08 |
 *    0 +----+----o----+----+
 *      | 14 | 03 | 00 | 07 |
 * -150 +----+----+----+----+
 *      | 15 | 04 | 05 | 06 |
 * -300 +----+----+----+----+
 *
 * For all of the distance based conversions, the grid cells are assumed to be squares,
 * even though the grid to spiral ID conversion would work just as well with rectangular cells.
 *
 * Note: Functions converting spiral IDs to points, locations, bounds, etc are more expensive O(n)
 * than conversions in the opposite direction (e.g. from coordinate to ID) that have runtime O(1).
 */
UCLASS(BlueprintType)
class OUURUNTIME_API USpiralIdUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Convert grid coordinates to spiral coordinates.
	 * Note that this uses grid coordinates as in grid cells, so the world coordinates will likely be different from X
	 * and Y.
	 */
	UFUNCTION(BlueprintPure)
	static int32 ConvertCoordinatesToSpiralId(const int32 X, const int32 Y);

	/**
	 * Convert grid coordinates to spiral coordinates.
	 * Note that this uses grid coordinates as in grid cells, so the world coordinates will likely be different from X
	 * and Y.
	 */
	UFUNCTION(BlueprintPure)
	static int32 ConvertCoordinatePointToSpiralId(const FIntPoint& Point);

	/**
	 * Convert a world location in 2D coordinates to a spiral ID.
	 *
	 * @param	Location			The location to convert
	 * @param	GridSize			Width of the grid cells
	 * @param	CoordinateSystem	The type of coordinate system that should be used
	 */
	UFUNCTION(BlueprintPure)
	static int32 ConvertWorldLocation2DToSpiralId(
		const FVector2D& Location,
		float GridSize,
		ESpiralCoordinateSystemType CoordinateSystem);

	/**
	 * Convert a world location in 3D space into a spiral ID.
	 * The Z component is ignored for this conversion so this is merely a convenience function.
	 *
	 * @param	Location			The location to convert
	 * @param	GridSize			Width of the grid cells
	 * @param	CoordinateSystem	The type of coordinate system that should be used
	 */
	UFUNCTION(BlueprintPure)
	static int32 ConvertWorldLocationToSpiralId(
		const FVector& Location,
		float GridSize,
		ESpiralCoordinateSystemType CoordinateSystem);

	/**
	 * Convert a spiral ID to grid coordinates.
	 * Note: This function is more expensive O(n) compared to the opposite conversion from coordinate to ID O(1).
	 */
	UFUNCTION(BlueprintPure)
	static FIntPoint ConvertSpiralIdToCoordinates(const int32 SpiralId);

	/**
	 * Convert a spiral ID to the center location of a grid cell in world space.
	 * Note: This function is more expensive O(n) compared to the opposite conversion from location to ID O(1).
	 *
	 * @param	SpiralId			Spiral Id of the cell to convert
	 * @param	GridSize			Width of the grid cells
	 * @param	CoordinateSystem	The type of coordinate system that should be used
	 * @returns						Center location of the cell in world space
	 */
	UFUNCTION(BlueprintPure)
	static FVector2D ConvertSpiralIdToCenterLocation(
		const int32 SpiralId,
		float GridSize,
		ESpiralCoordinateSystemType CoordinateSystem);

	/**
	 * Convert a spiral ID to the 2D bounds of a cell in world space.
	 * Note: This function is more expensive O(n) compared to conversions from coordinate to ID O(1).
	 *
	 * @param	SpiralId			Spiral Id of the cell to convert
	 * @param	GridSize			Width of the grid cells
	 * @param	CoordinateSystem	The type of coordinate system that should be used
	 * @returns						Two dimensional bound of the cell
	 */
	UFUNCTION(BlueprintPure)
	static FBox2D ConvertSpiralIdToBounds(
		const int32 SpiralId,
		const float GridSize,
		ESpiralCoordinateSystemType CoordinateSystem);

	/**
	 * Convert a spiral ID to the 3D bounds of a cell in world space.
	 * Because the spiral ID only describes the 2D location of the cell,
	 * the height and elevation have to be supplied by the function caller.
	 * Note: This function is more expensive O(n) compared to conversions from coordinate to ID O(1).
	 *
	 * @param	SpiralId			Spiral Id of the cell to convert
	 * @param	GridSize			Width of the grid cells
	 * @param	CoordinateSystem	The type of coordinate system that should be used
	 * @param	BoundsHeight		Height of the grid cell
	 * @param	BoundsElevation		Distance between Z=0 and bottom surface of the grid cell
	 * @returns						Two dimensional bound of the cell
	 */
	UFUNCTION(BlueprintPure)
	static FBox ConvertSpiralIdToBounds3D(
		const int32 SpiralId,
		const float GridSize,
		ESpiralCoordinateSystemType CoordinateSystem,
		const float BoundsHeight,
		const float BoundsElevation);
};
