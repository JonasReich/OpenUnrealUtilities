// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "JsonDataCustomVersions.generated.h"

class FJsonObject;

/**
 * Store map of custom versions required for a json data asset file.
 */
USTRUCT()
struct OUURUNTIME_API FJsonDataCustomVersions
{
	GENERATED_BODY()

public:
	FJsonDataCustomVersions() = default;
	FJsonDataCustomVersions(const TSet<FGuid>& CustomVersionGuids);

	int32 GetCustomVersion(const FGuid& CustomVersionGuid) const;

	void EnsureExpectedVersions(const TSet<FGuid>& CustomVersionGuids);

	TSharedPtr<FJsonObject> ToJsonObject() const;
	void ReadFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject);

private:
	UPROPERTY(EditDefaultsOnly)
	TMap<FGuid, int32> VersionsByGuid;
};
