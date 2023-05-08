// Copyright (c) 2022 Jonas Reich

#include "JsonDataAsset/JsonDataAssetPath.h"

#include "JsonDataAsset/JsonDataAsset.h"
#include "OUURuntimeVersion.h"

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
	Ar << Path;
	return true;
}

bool FJsonDataAssetPath::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FOUURuntimeVersion::k_GUID);

	if (Ar.CustomVer(FOUURuntimeVersion::k_GUID) >= FOUURuntimeVersion::InitialVersion)
	{
		FSoftObjectPath ActualPath = Path.ToSoftObjectPath();
		Ar << ActualPath;

		if (Ar.IsLoading())
		{
			Path = MoveTemp(ActualPath);
		}
	}
	else
	{
		if (StaticStruct()->UseBinarySerialization(Ar))
		{
			StaticStruct()->SerializeBin(Ar, this);
		}
		else
		{
			StaticStruct()->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(this), StaticStruct(), nullptr);
		}
	}

	return true;
}

bool FJsonDataAssetPath::Serialize(FStructuredArchive::FSlot Slot)
{
	Slot.GetUnderlyingArchive().UsingCustomVersion(FOUURuntimeVersion::k_GUID);

	if (Slot.GetUnderlyingArchive().CustomVer(FOUURuntimeVersion::k_GUID) >= FOUURuntimeVersion::InitialVersion)
	{
		FSoftObjectPath ActualPath = Path.ToSoftObjectPath();
		Slot << ActualPath;

		if (Slot.GetUnderlyingArchive().IsLoading())
		{
			Path = MoveTemp(ActualPath);
		}
	}
	else
	{
		if (StaticStruct()->UseBinarySerialization(Slot.GetUnderlyingArchive()))
		{
			StaticStruct()->SerializeBin(Slot, this);
		}
		else
		{
			StaticStruct()->SerializeTaggedProperties(Slot, reinterpret_cast<uint8*>(this), StaticStruct(), nullptr);
		}
	}

	return true;
}