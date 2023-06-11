// Copyright (c) 2023 Jonas Reich & Contributors

#include "JsonDataAsset/JsonDataAssetPointers.h"

#include "OUURuntimeVersion.h"

//---------------------------------------------------------------------------------------------------------------------

FSoftJsonDataAssetPtr::FSoftJsonDataAssetPtr(FJsonDataAssetPath InPath) : Path(MoveTemp(InPath)) {}

FSoftJsonDataAssetPtr::FSoftJsonDataAssetPtr(const UJsonDataAsset* Object) : Path(Object) {}

UJsonDataAsset* FSoftJsonDataAssetPtr::LoadSynchronous() const
{
	return Path.LoadSynchronous();
}

bool FSoftJsonDataAssetPtr::ImportTextItem(
	const TCHAR*& Buffer,
	int32 PortFlags,
	UObject* Parent,
	FOutputDevice* ErrorText)
{
	return Path.ImportTextItem(Buffer, PortFlags, Parent, ErrorText);
}

bool FSoftJsonDataAssetPtr::ExportTextItem(
	FString& ValueStr,
	FSoftJsonDataAssetPtr const& DefaultValue,
	UObject* Parent,
	int32 PortFlags,
	UObject* ExportRootScope) const
{
	return Path.ExportTextItem(ValueStr, DefaultValue.Path, Parent, PortFlags, ExportRootScope);
}

bool FSoftJsonDataAssetPtr::SerializeFromMismatchedTag(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot)
{
	return Path.SerializeFromMismatchedTag(Tag, Slot);
}

bool FSoftJsonDataAssetPtr::NetSerialize(FArchive& Ar, UPackageMap* PackageMap, bool& OutSuccess)
{
	return Path.NetSerialize(Ar, PackageMap, OutSuccess);
}

bool FSoftJsonDataAssetPtr::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FOUURuntimeVersion::k_GUID);

	if (Ar.CustomVer(FOUURuntimeVersion::k_GUID) >= FOUURuntimeVersion::InitialVersion)
	{
		return Path.Serialize(Ar);
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
		return true;
	}
}

bool FSoftJsonDataAssetPtr::Serialize(FStructuredArchive::FSlot Slot)
{
	Slot.GetUnderlyingArchive().UsingCustomVersion(FOUURuntimeVersion::k_GUID);

	if (Slot.GetUnderlyingArchive().CustomVer(FOUURuntimeVersion::k_GUID) >= FOUURuntimeVersion::InitialVersion)
	{
		return Path.Serialize(Slot);
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
		return true;
	}
}

//---------------------------------------------------------------------------------------------------------------------

FJsonDataAssetPtr::FJsonDataAssetPtr(FJsonDataAssetPath InPath) :
	Path(MoveTemp(InPath)), HardReference(Path.LoadSynchronous())
{
}

FJsonDataAssetPtr::FJsonDataAssetPtr(const UJsonDataAsset* Object) :
	Path(Object), HardReference(const_cast<UJsonDataAsset*>(Object))
{
}

#if WITH_EDITOR
void FJsonDataAssetPtr::NotifyPathChanged()
{
	HardReference = Path.ResolveObject();
}
#endif

bool FJsonDataAssetPtr::ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText)
{
	if (Path.ImportTextItem(Buffer, PortFlags, Parent, ErrorText))
	{
		HardReference = Path.LoadSynchronous();
		return true;
	}

	HardReference = nullptr;

	return false;
}

bool FJsonDataAssetPtr::ExportTextItem(
	FString& ValueStr,
	FJsonDataAssetPtr const& DefaultValue,
	UObject* Parent,
	int32 PortFlags,
	UObject* ExportRootScope) const
{
	return Path.ExportTextItem(ValueStr, DefaultValue.Path, Parent, PortFlags, ExportRootScope);
}

bool FJsonDataAssetPtr::SerializeFromMismatchedTag(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot)
{
	if (Path.SerializeFromMismatchedTag(Tag, Slot))
	{
		HardReference = Path.ResolveObject();
		return true;
	}

	return false;
}

bool FJsonDataAssetPtr::NetSerialize(FArchive& Ar, UPackageMap* PackageMap, bool& OutSuccess)
{
	if (Path.NetSerialize(Ar, PackageMap, OutSuccess))
	{
		if (Ar.IsLoading())
		{
			HardReference = Path.ResolveObject();
		}

		return true;
	}

	return false;
}

bool FJsonDataAssetPtr::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FOUURuntimeVersion::k_GUID);

	bool Success = true;
	if (Ar.CustomVer(FOUURuntimeVersion::k_GUID) >= FOUURuntimeVersion::InitialVersion)
	{
		Success = Path.Serialize(Ar);
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

	if (Success && Ar.IsLoading())
	{
		HardReference = Path.ResolveObject();
	}

	return Success;
}

bool FJsonDataAssetPtr::Serialize(FStructuredArchive::FSlot Slot)
{
	auto& UnderlyingArchive = Slot.GetUnderlyingArchive();
	UnderlyingArchive.UsingCustomVersion(FOUURuntimeVersion::k_GUID);

	bool Success = true;
	if (UnderlyingArchive.CustomVer(FOUURuntimeVersion::k_GUID) >= FOUURuntimeVersion::InitialVersion)
	{
		Success = Path.Serialize(Slot);
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

	if (Success && UnderlyingArchive.IsLoading())
	{
		HardReference = Path.ResolveObject();
	}

	return Success;
}
