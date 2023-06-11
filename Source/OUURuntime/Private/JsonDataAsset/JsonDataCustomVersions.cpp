// Copyright (c) 2023 Jonas Reich & Contributors

#include "JsonDataAsset/JsonDataCustomVersions.h"

#include "Dom/JsonObject.h"

FJsonDataCustomVersions::FJsonDataCustomVersions(const TSet<FGuid>& CustomVersionGuids)
{
	for (const auto& Guid : CustomVersionGuids)
	{
		auto OptVersion = FCurrentCustomVersions::Get(Guid);
		if (ensureMsgf(
				OptVersion.IsSet(),
				TEXT("Version GUID '%s' provided for json data asset is not registered as a custom version."),
				*Guid.ToString()))
		{
			VersionsByGuid.Add(Guid, OptVersion.GetValue().Version);
		}
	}
}

int32 FJsonDataCustomVersions::GetCustomVersion(const FGuid& CustomVersionGuid) const
{
	const auto Version = VersionsByGuid.Find(CustomVersionGuid);
	if (ensureMsgf(
			Version,
			TEXT("Tried to access custom version '%s' from json data which was not registered via GetCustomVersions."),
			*CustomVersionGuid.ToString()))
	{
		return *Version;
	}

	return -1;
}

void FJsonDataCustomVersions::EnsureExpectedVersions(const TSet<FGuid>& CustomVersionGuids)
{
	for (const auto& Guid : CustomVersionGuids)
	{
		VersionsByGuid.FindOrAdd(Guid, -1);
	}
}

TSharedPtr<FJsonObject> FJsonDataCustomVersions::ToJsonObject() const
{
	auto JsonObject = MakeShared<FJsonObject>();

	for (const auto& Entry : VersionsByGuid)
	{
		JsonObject->SetNumberField(Entry.Key.ToString(), Entry.Value);
	}

	return JsonObject;
}

void FJsonDataCustomVersions::ReadFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject)
{
	VersionsByGuid.Reset();

	if (JsonObject)
	{
		for (const auto& Entry : JsonObject->Values)
		{
			VersionsByGuid.Add(FGuid(Entry.Key), JsonObject->GetIntegerField(Entry.Key));
		}
	}
}
