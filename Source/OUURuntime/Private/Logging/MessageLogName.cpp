// Copyright (c) 2022 Jonas Reich

#include "Logging/MessageLogName.h"

FName GetMessageLogName(EMessageLogName MesageLogName)
{
#define MESSAGELOG_ENUM_TO_FNAME_CASE(Entry)                                                                           \
	case EMessageLogName::Entry: return PREPROCESSOR_TO_STRING(Entry);
	switch (MesageLogName)
	{
		MESSAGELOG_ENUM_TO_FNAME_CASE(PIE)
		MESSAGELOG_ENUM_TO_FNAME_CASE(EditorErrors)
		MESSAGELOG_ENUM_TO_FNAME_CASE(AssetTools)
		MESSAGELOG_ENUM_TO_FNAME_CASE(MapCheck)
		MESSAGELOG_ENUM_TO_FNAME_CASE(AssetCheck)
		MESSAGELOG_ENUM_TO_FNAME_CASE(AssetReimport)
		MESSAGELOG_ENUM_TO_FNAME_CASE(AnimBlueprintLog)
		MESSAGELOG_ENUM_TO_FNAME_CASE(AutomationTestingLog)
		MESSAGELOG_ENUM_TO_FNAME_CASE(BuildAndSubmitErrors)
		MESSAGELOG_ENUM_TO_FNAME_CASE(Blueprint)
		MESSAGELOG_ENUM_TO_FNAME_CASE(BlueprintLog)
		MESSAGELOG_ENUM_TO_FNAME_CASE(HLODResults)
		MESSAGELOG_ENUM_TO_FNAME_CASE(LightingResults)
		MESSAGELOG_ENUM_TO_FNAME_CASE(LoadErrors)
		MESSAGELOG_ENUM_TO_FNAME_CASE(LocalizationService)
		MESSAGELOG_ENUM_TO_FNAME_CASE(PackagingResults)
		MESSAGELOG_ENUM_TO_FNAME_CASE(SlateStyleLog)
		MESSAGELOG_ENUM_TO_FNAME_CASE(SourceControl)
		MESSAGELOG_ENUM_TO_FNAME_CASE(TranslationEditor)
		MESSAGELOG_ENUM_TO_FNAME_CASE(UDNParser)
		MESSAGELOG_ENUM_TO_FNAME_CASE(WidgetEvents)
		// return Blueprint log by default because it matches most situations
	default: return "Blueprint";
	}
#undef MESSAGELOG_ENUM_TO_FNAME_CASE
}
