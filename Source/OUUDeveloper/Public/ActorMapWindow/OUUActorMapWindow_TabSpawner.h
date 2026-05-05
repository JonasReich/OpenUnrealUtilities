// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Actor.h"
#include "Templates/BitmaskUtils.h"
#include "ActorMapWindow/OUUActorMapQuery.h"

namespace OUU::Developer::ActorMapWindow
{
	extern OUUDEVELOPER_API FName GTabName;
	extern OUUDEVELOPER_API FText GTabTitle;

	void OUUDEVELOPER_API RegisterNomadTabSpawner();
	void OUUDEVELOPER_API UnregisterNomadTabSpawner();
	void OUUDEVELOPER_API TryInvokeTab();
	
	// Show flags for the main overlay window
	enum class EShowFlags
	{
		Labels = 0b1,
		SceneCapture = 0b10,
		Distance = 0b100,

		Default = Labels | SceneCapture
	};
} // namespace OUU::Developer::ActorMapWindow

DECLARE_BITMASK_OPERATORS(OUU::Developer::ActorMapWindow::EShowFlags)