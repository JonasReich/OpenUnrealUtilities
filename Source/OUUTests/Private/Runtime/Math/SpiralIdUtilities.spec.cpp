// Copyright (c) 2021 Jonas Reich

#include "OUUTestUtilities.h"
#include "Math/SpiralIdUtilities.h"

#if WITH_AUTOMATION_WORKER

// Entries are ordered how the numbers are seen top-down in UE4 (left handed coordinate system).
// This means the y index has to be negated in the test below.
const TArray<TArray<int32>> SampleGrid =
{
	// x-offset:
	//            v
	//   -1  -2  -3           // y-offset:
	{35, 16, 17, 18, 19, 20}, //  0
	{34, 15, 4, 5, 6, 21},    // -1
	{33, 14, 3, 0, 7, 22},    // -2  <
	{32, 13, 2, 1, 8, 23},
	{31, 12, 11, 10, 9, 24},
	{30, 29, 28, 27, 26, 25}
};

// Center location of tiles with 15000 size.
const TMap<int32, FVector2D> SampleBounds =
{
	{0, {7500, -7500}},
	{1, {7500, 7500}},
	{107, {37500, -82500}},
	{130, {-52500, 82500}},
	{237, {-67500, 112500}}
};

BEGIN_DEFINE_SPEC(FSpiralIdUtilitiesSpec, "OpenUnrealUtilities.Math.SpiralIdUtilities", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FSpiralIdUtilitiesSpec)

void FSpiralIdUtilitiesSpec::Define()
{
	It("ConvertCoordinatesToSpiralId / ConvertSpiralIdToCoordinates should convert all number from "
		"6x6 test grid as expected", [this]()
	{
		for (int32 i = 0; i < 6; i++)
		{
			for (int32 j = 0; j < 6; j++)
			{
				const int32 y = (i - 2) * -1; // flip y axis for right handed coordinate system
				const int32 x = j - 3;
				const int32 ExpectedSpiralId = SampleGrid[i][j];

				const int32 Result = USpiralIdUtilities::ConvertCoordinatesToSpiralId(x, y);
				SPEC_TEST_EQUAL(Result, ExpectedSpiralId);

				const FIntPoint PointResult = USpiralIdUtilities::ConvertSpiralIdToCoordinates(ExpectedSpiralId);
				SPEC_TEST_EQUAL(PointResult.X, x);
				SPEC_TEST_EQUAL(PointResult.Y, y);
			}
		}
	});

	It("ConvertWorldLocation2DToSpiralId / ConvertSpiralIdToCenterLocation should "
		"convert parcel IDs to/from cell center locations as expected", [this]()
	{
		for (auto& Entry : SampleBounds)
		{
			const int32 ExpectedSpiralId = Entry.Key;
			const FVector2D ExpectedLocation = Entry.Value;
			const float GridSize = 15000;
			const FVector2D ResultLocation = USpiralIdUtilities::ConvertSpiralIdToCenterLocation(ExpectedSpiralId, GridSize);
			const int32 ResultSpiralId = USpiralIdUtilities::ConvertWorldLocation2DToSpiralId(ExpectedLocation, GridSize);

			SPEC_TEST_EQUAL(ResultSpiralId, ExpectedSpiralId);
			SPEC_TEST_EQUAL(ResultLocation, ExpectedLocation);
		}
	});
};

#endif
