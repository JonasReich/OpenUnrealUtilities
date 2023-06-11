// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "JsonDAtaAsset/JsonDataAsset.h"

// Global utility functions for json data asset system.
namespace OUU::Runtime::JsonData
{
	// If true, a separate package root is used for source files.
	OUURUNTIME_API bool ShouldUseSeparateSourceMountRoot();

	OUURUNTIME_API extern const FName GameRootName;

	OUURUNTIME_API FString GetSourceRoot_Full(const FName& RootName, EJsonDataAccessMode AccessMode);

#if WITH_EDITOR
	// Mount point for source files. Not required at runtime, but for some content browser functionality.
	// #TODO move into content browser source. This is specific to which mount is investigated!!!!
	OUURUNTIME_API FString GetSourceMountPointRoot_Package(const FName& RootName);
	OUURUNTIME_API FString GetSourceMountPointRoot_DiskFull(const FName& RootName);
#endif

	// Mount point for generated packages.
	// Save into Save dir, so the packages are not versioned and can safely be deleted on engine startup.
	OUURUNTIME_API FString GetCacheMountPointRoot_Package(const FName& RootName);
	OUURUNTIME_API FString GetCacheMountPointRoot_DiskFull(const FName& RootName);

	OUURUNTIME_API bool PackageIsJsonData(const FString& PackagePath);

	OUURUNTIME_API FString PackageToDataRelative(const FString& PackagePath);

	OUURUNTIME_API FString PackageToSourceFull(const FString& PackagePath, EJsonDataAccessMode AccessMode);

	// Take a path that is relative to the project root and convert it into a package path.
	OUURUNTIME_API FString SourceFullToPackage(const FString& FullPath, EJsonDataAccessMode AccessMode);

	OUURUNTIME_API FString PackageToObjectName(const FString& Package);

	OUURUNTIME_API bool ShouldIgnoreInvalidExtensions();

	OUURUNTIME_API bool ShouldReadFromCookedContent();
	OUURUNTIME_API bool ShouldWriteToCookedContent();
	OUURUNTIME_API void CheckJsonPaths();

	namespace Private
	{
		// not API exposed, because it's only for internal use
		void Delete(const FString& PackagePath);
	} // namespace Private
} // namespace OUU::Runtime::JsonData
