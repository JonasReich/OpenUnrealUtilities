// Copyright (c) 2021 Jonas Reich

#include "Math/SpiralIdUtilities.h"

int32 USpiralIdUtilities::ConvertCoordinatesToSpiralId(const int32 X, const int32 Y)
{
	const int32 A = FMath::Abs(FMath::Abs(X) - FMath::Abs(Y)) + FMath::Abs(X) + FMath::Abs(Y);
	// Increase input by 0.1 to get only non-zero signs (-1 or +1) but never 0!
	const int32 Sign = StaticCast<int32>(FMath::Sign<double>(0.1 + X + Y));
	return A * A + Sign * (A + X - Y);
}

int32 USpiralIdUtilities::ConvertCoordinatePointToSpiralId(const FIntPoint& Point)
{
	return ConvertCoordinatesToSpiralId(Point.X, Point.Y);
}

int32 USpiralIdUtilities::ConvertWorldLocation2DToSpiralId(const FVector2D& Location, float GridSize)
{
	return ConvertCoordinatesToSpiralId(
		FMath::FloorToInt(Location.X / GridSize),
		// Flip Y axis for left handed UE4 coordinate system
		FMath::FloorToInt(-Location.Y / GridSize));
}

int32 USpiralIdUtilities::ConvertWorldLocationToSpiralId(const FVector& Location, float GridSize)
{
	return ConvertCoordinatesToSpiralId(
		FMath::FloorToInt(Location.X / GridSize),
		// Flip Y axis for left handed UE4 coordinate system
		FMath::FloorToInt(-Location.Y / GridSize));
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

FVector2D USpiralIdUtilities::ConvertSpiralIdToCenterLocation(const int32 SpiralId, const float GridSize)
{
	FIntPoint Coordinates = ConvertSpiralIdToCoordinates(SpiralId);
	// Flip Y axis for left handed UE4 coordinate system
	Coordinates.Y *= -1;
	FVector2D Result(0.5f, -0.5f);
	Result += Coordinates;
	Result *= GridSize;
	return Result;
}

FBox2D USpiralIdUtilities::ConvertSpiralIdToBounds(const int32 SpiralId, const float GridSize)
{
	FIntPoint MinPoint = ConvertSpiralIdToCoordinates(SpiralId);
	// Flip Y axis for left handed UE4 coordinate system
	MinPoint.Y *= -1;
	const FIntPoint MaxPoint = MinPoint + FIntPoint(1,-1);
	return {MinPoint * GridSize, MaxPoint * GridSize};
}

FBox USpiralIdUtilities::ConvertSpiralIdToBounds3D(const int32 SpiralId, const float GridSize, const float BoundsHeight, const float BoundsElevation)
{
	const FBox2D Box2D = ConvertSpiralIdToBounds(SpiralId, GridSize);
	return FBox (FVector(Box2D.Min.X, Box2D.Min.Y, BoundsElevation), FVector(Box2D.Max.X, Box2D.Max.Y, BoundsElevation + BoundsHeight));
}
