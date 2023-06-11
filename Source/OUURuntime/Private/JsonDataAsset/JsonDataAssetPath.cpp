// Copyright (c) 2023 Jonas Reich & Contributors

#include "JsonDataAsset/JsonDataAssetPath.h"

#include "JsonDataAsset/JsonDataAsset.h"
#include "JsonDataAsset/JsonDataAssetSubsystem.h"
#include "OUURuntimeVersion.h"
#include "Templates/StructSerializationHelpers.h"

FJsonDataAssetPath::FJsonDataAssetPath(const UJsonDataAsset* Object) : Path(Object) {}

UJsonDataAsset* FJsonDataAssetPath::ResolveObject() const
{
	// TSoftObjectPtr::Get() is more or less the same as FSoftObjectPath::ResolveObject.
	// This function was called Get() previously, but this lead to confusion.
	return Path.Get();
}

UJsonDataAsset* FJsonDataAssetPath::LoadSynchronous() const
{
	// This attempts to find the object in memory (Path.LoadSynchronous)
	// OR load cached generated asset (editor only).
	auto* ExistingAsset = Path.LoadSynchronous();
	// If the LoadSynchronous call above failed, we need to create a new package / in-memory object via the internals
	return ExistingAsset ? ExistingAsset : UJsonDataAsset::LoadJsonDataAsset_Internal(*this, nullptr);
}

UJsonDataAsset* FJsonDataAssetPath::ForceReload() const
{
	// This always resets + reloads member data.
	// Make sure to re-use an existing object if possible.
	auto* ExistingAsset = ResolveObject();
	return UJsonDataAsset::LoadJsonDataAsset_Internal(*this, ExistingAsset);
}

void FJsonDataAssetPath::SetPackagePath(const FString& InPackagePath)
{
	ensureMsgf(
		InPackagePath.Contains(TEXT(".")) == false,
		TEXT("SetPackagePath must be called with package paths, but '%s' contains a colon, indicating it's an object "
			 "path!"),
		*InPackagePath);

	auto ObjectName = OUU::Runtime::JsonData::PackageToObjectName(InPackagePath);
	Path = FSoftObjectPath(FString::Printf(TEXT("%s.%s"), *InPackagePath, *ObjectName));
}

void FJsonDataAssetPath::SetObjectPath(const FString& InObjectPath)
{
	Path = FSoftObjectPath(InObjectPath);
}

void FJsonDataAssetPath::SetFromString(const FString& InString)
{
	int32 Index = INDEX_NONE;
	InString.FindLastChar(TCHAR('.'), OUT Index);
	if (Index == INDEX_NONE)
	{
		SetPackagePath(InString);
	}
	else
	{
		SetObjectPath(InString);
	}
}

bool FJsonDataAssetPath::ImportTextItem(
	const TCHAR*& Buffer,
	int32 PortFlags,
	UObject* Parent,
	FOutputDevice* ErrorText)
{
	SetFromString(Buffer);

	return true;
}

bool FJsonDataAssetPath::ExportTextItem(
	FString& ValueStr,
	FJsonDataAssetPath const& DefaultValue,
	UObject* Parent,
	int32 PortFlags,
	UObject* ExportRootScope) const
{
	ValueStr = GetPackagePath();
	return true;
}

bool FJsonDataAssetPath::SerializeFromMismatchedTag(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot)
{
	if (Tag.Type == NAME_ObjectProperty)
	{
		UObject* pOldTarget = nullptr;
		Slot << pOldTarget;
		Path = pOldTarget;
		return true;
	}
	else if (Tag.Type == NAME_SoftObjectProperty)
	{
		FSoftObjectPath OldTarget;
		Slot << OldTarget;
		Path = OldTarget;
		return true;
	}

	return false;
}

bool FJsonDataAssetPath::NetSerialize(FArchive& Ar, UPackageMap* PackageMap, bool& OutSuccess)
{
	// We must write the soft object path instead of the pointer itself since our UJsonDataAsset is marked as not
	// supported for networking. The serialization of FSoftObjectPtr will fail because of this if the object is
	// currently loaded, but FSoftObjectPath does not perform this check.
	UJsonDataAssetSubsystem::NetSerializePath(*this, Ar);

	return true;
}

bool FJsonDataAssetPath::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FOUURuntimeVersion::k_GUID);

	if (Ar.CustomVer(FOUURuntimeVersion::k_GUID) >= FOUURuntimeVersion::AddedJsonDataAssetPathSerialization)
	{
		FSoftObjectPath ActualPath = Path.ToSoftObjectPath();

		if (Ar.IsSaving())
		{
			// To fixup redirectors - not ideal, but get's the job done
			if (auto* ActualObject = Path.Get())
			{
				ActualPath = ActualObject;
			}
		}

		Ar << ActualPath;

		if (Ar.IsLoading())
		{
			Path = MoveTemp(ActualPath);
		}
	}
	else
	{
		OUU::Runtime::DefaultStructSerialization(*this, Ar);
	}

	return true;
}

bool FJsonDataAssetPath::Serialize(FStructuredArchive::FSlot Slot)
{
	Slot.GetUnderlyingArchive().UsingCustomVersion(FOUURuntimeVersion::k_GUID);

	if (Slot.GetUnderlyingArchive().CustomVer(FOUURuntimeVersion::k_GUID)
		>= FOUURuntimeVersion::AddedJsonDataAssetPathSerialization)
	{
		FSoftObjectPath ActualPath = Path.ToSoftObjectPath();

		if (Slot.GetUnderlyingArchive().IsSaving())
		{
			// To fixup redirectors - not ideal, but get's the job done
			if (auto* ActualObject = Path.Get())
			{
				ActualPath = ActualObject;
			}
		}

		Slot << ActualPath;

		if (Slot.GetUnderlyingArchive().IsLoading())
		{
			Path = MoveTemp(ActualPath);
		}
	}
	else
	{
		OUU::Runtime::DefaultStructSerialization(*this, Slot);
	}

	return true;
}
