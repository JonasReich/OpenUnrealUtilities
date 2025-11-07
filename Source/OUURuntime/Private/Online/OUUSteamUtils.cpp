// Copyright (c) 2025 Jonas Reich & Contributors

#include "Online/OUUSteamUtils.h"

#include "LogOpenUnrealUtilities.h"
#include "Serialization/BufferArchive.h"

#if WITH_STEAM
	#include "SteamSharedModule.h"

FAutoConsoleCommand CCommand_StartSteamAPI{
	TEXT("ouu.steam.StartSteamAPI"),
	TEXT("Write the Steam AppID to disk and startup the Steam API"),
	FConsoleCommandDelegate::CreateLambda([]() {
		UOUUSteamUtils::WriteSteamAppIdToDisk();
		auto ScopedSteamAPIClientHandle = FSteamSharedModule::Get().ObtainSteamClientInstanceHandle();
	})};
#endif

FString UOUUSteamUtils::GetSteamAppIdFilename()
{
	return FString::Printf(TEXT("%s%s"), FPlatformProcess::BaseDir(), TEXT("steam_appid.txt"));
}

bool UOUUSteamUtils::WriteSteamAppIdToDisk(int32 SteamAppId)
{
	if (SteamAppId > 0)
	{
		// Access the physical file writer directly so that we still write next to the executable in CotF builds.
		const FString SteamAppIdFilename = GetSteamAppIdFilename();
		IFileHandle* Handle = IPlatformFile::GetPlatformPhysical().OpenWrite(*SteamAppIdFilename, false, false);
		if (!Handle)
		{
			UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Failed to create file: %s"), *SteamAppIdFilename);
			return false;
		}
		else
		{
			const FString AppId = FString::Printf(TEXT("%d"), SteamAppId);

			FBufferArchive Archive;
			Archive.Serialize((void*)TCHAR_TO_ANSI(*AppId), AppId.Len());

			Handle->Write(Archive.GetData(), Archive.Num());
			delete Handle;
			Handle = nullptr;

			return true;
		}
	}

	UE_LOG(
		LogOpenUnrealUtilities,
		Warning,
		TEXT("Steam App Id provided (%d) is invalid, must be greater than 0!"),
		SteamAppId);
	return false;
}

bool UOUUSteamUtils::WriteSteamAppIdToDisk()
{
	int32 AppID = INDEX_NONE;
	if (GConfig->GetInt(TEXT("OnlineSubsystemSteam"), TEXT("SteamDevAppId"), AppID, GEngineIni) == false)
	{
		UE_LOG(
			LogOpenUnrealUtilities,
			Warning,
			TEXT("Missing SteamDevAppId key in OnlineSubsystemSteam of DefaultEngine.ini"));
		return false;
	}
	else if (WriteSteamAppIdToDisk(AppID) == false)
	{
		UE_LOG(
			LogOpenUnrealUtilities,
			Warning,
			TEXT("Could not create/update the steam_appid.txt file! Make sure the directory is writable and "
				 "there isn't another instance using this file"));
		return false;
	}

	return true;
}
