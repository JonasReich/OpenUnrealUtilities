// Copyright (c) 2023 Jonas Reich & Contributors

#include "OUURuntimeVersion.h"

// Unique OUURuntime version id
const FGuid FOUURuntimeVersion::k_GUID("0E26539A-1A69-4EAE-81CE-70D356B69D52");
// Register OUURuntime custom version with core
FDevVersionRegistration GRegisterOUURuntimeVersion(
	FOUURuntimeVersion::k_GUID,
	FOUURuntimeVersion::LatestVersion,
	TEXT("OUURuntime"));