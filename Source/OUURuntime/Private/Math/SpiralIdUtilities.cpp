// Copyright (c) 2023 Jonas Reich & Contributors

#include "Math/SpiralIdUtilities.h"

int32 USpiralIdUtilities::ConvertCoordinatesToSpiralId(const int32 X, const int32 Y)
{
	// If you find this calculation confusing, don't worry I had a hard time understanding it
	// after a few months as well.
	// There's a material in /OpenUnrealUtilities/Materials/M_SpiralID_Visualization
	// which has a visual breakdown of this calculation.

	const int32 ManhattanDistance = FMath::Abs(X) + FMath::Abs(Y);
	const int32 DistanceFromDiagonals = FMath::Abs(FMath::Abs(X) - FMath::Abs(Y));
	const int32 RingDistance = DistanceFromDiagonals + ManhattanDistance;
	// Make a diagonal through the origin through (-1,1) and (1,-1) and
	// pixels that have DiagSign = 1 will be in the half that contains (1,1).
	// pixels that have DiagSign = -1 will be in the half that contains (-1,-1).
	// Increase input by 0.1 to get only non-zero signs (-1 or +1) but never 0!
	const int32 DiagSign = StaticCast<int32>(FMath::Sign<double>(0.1 + X + Y));

	// This is the magic bit. I don't really know how to put this into words.
	// Can potentially be optimized by a math nerd?
	return (RingDistance * RingDistance) + (DiagSign * ((RingDistance + X) - Y));
}

int32 USpiralIdUtilities::ConvertCoordinatePointToSpiralId(const FIntPoint& Point)
{
	return ConvertCoordinatesToSpiralId(Point.X, Point.Y);
}

int32 USpiralIdUtilities::ConvertWorldLocation2DToSpiralId(
	const FVector2D& Location,
	float GridSize,
	ESpiralCoordinateSystemType CoordinateSystem)
{
	const int32 X = FMath::FloorToInt(Location.X / GridSize);
	int32 Y = 0;
	switch (CoordinateSystem)
	{
	case ESpiralCoordinateSystemType::RightHanded: Y = FMath::FloorToInt(Location.Y / GridSize); break;
	case ESpiralCoordinateSystemType::LeftHanded: Y = FMath::FloorToInt(-Location.Y / GridSize); break;
	default:;
	}

	return ConvertCoordinatesToSpiralId(X, Y);
}

int32 USpiralIdUtilities::ConvertWorldLocationToSpiralId(
	const FVector& Location,
	float GridSize,
	ESpiralCoordinateSystemType CoordinateSystem)
{
	const int32 X = FMath::FloorToInt(Location.X / GridSize);
	int32 Y = 0;
	switch (CoordinateSystem)
	{
	case ESpiralCoordinateSystemType::RightHanded: Y = FMath::FloorToInt(Location.Y / GridSize); break;
	case ESpiralCoordinateSystemType::LeftHanded: Y = FMath::FloorToInt(-Location.Y / GridSize); break;
	default:;
	}

	return ConvertCoordinatesToSpiralId(X, Y);
}

FIntPoint USpiralIdUtilities::ConvertSpiralIdToCoordinates(const int32 SpiralId)
{
	FIntPoint PerStepDelta(0, 1);
	int32 SegmentLength = 1;

	FIntPoint Coordinates(0, 0);
	int32 SegmentProgress = 0;
	for (int i = 0; i < SpiralId; ++i)
	{
		Coordinates += PerStepDelta;
		++SegmentProgress;

		if (SegmentProgress == SegmentLength)
		{
			SegmentProgress = 0;

			// Rotate PerStepDelta by 90 deg
			const int32 Temp = PerStepDelta.X;
			PerStepDelta.X = -PerStepDelta.Y;
			PerStepDelta.Y = Temp;

			if (PerStepDelta.X == 0)
			{
				++SegmentLength;
			}
		}
	}
	Coordinates.Y *= -1;
	return Coordinates;
}

FVector2D USpiralIdUtilities::ConvertSpiralIdToCenterLocation(
	const int32 SpiralId,
	const float GridSize,
	ESpiralCoordinateSystemType CoordinateSystem)
{
	FIntPoint Coordinates = ConvertSpiralIdToCoordinates(SpiralId);
	if (CoordinateSystem == ESpiralCoordinateSystemType::LeftHanded)
	{
		Coordinates.Y *= -1;
	}

	FVector2D Result(0.5f, -0.5f);
	Result += Coordinates;
	Result *= GridSize;
	return Result;
}

FBox2D USpiralIdUtilities::ConvertSpiralIdToBounds(
	const int32 SpiralId,
	const float GridSize,
	ESpiralCoordinateSystemType CoordinateSystem)
{
	FIntPoint MinPoint = ConvertSpiralIdToCoordinates(SpiralId);
	if (CoordinateSystem == ESpiralCoordinateSystemType::LeftHanded)
	{
		MinPoint.Y *= -1;
	}
	const FIntPoint MaxPoint = MinPoint + FIntPoint(1, -1);
	return {MinPoint * GridSize, MaxPoint * GridSize};
}

FBox USpiralIdUtilities::ConvertSpiralIdToBounds3D(
	const int32 SpiralId,
	const float GridSize,
	ESpiralCoordinateSystemType CoordinateSystem,
	const float BoundsHeight,
	const float BoundsElevation)
{
	const FBox2D Box2D = ConvertSpiralIdToBounds(SpiralId, GridSize, CoordinateSystem);
	return FBox(
		FVector(Box2D.Min.X, Box2D.Min.Y, BoundsElevation),
		FVector(Box2D.Max.X, Box2D.Max.Y, BoundsElevation + BoundsHeight));
}
