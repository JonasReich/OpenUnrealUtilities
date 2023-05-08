// Copyright by Grimlore Games & THQ Nordic

#pragma once

#include "CoreMinimal.h"

#include "UObject/NoExportTypes.h"

// Custom serialization version for changes made in the OUURuntime module.
struct FOUURuntimeVersion
{
public:
	enum Type
	{
		// Before any version changes were made
		InitialVersion = 0,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
	const static FGuid k_GUID;

private:
	FOUURuntimeVersion() = delete;
};