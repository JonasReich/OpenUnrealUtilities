// Copyright (c) 2023 Jonas Reich & Contributors

#include "GameplayDebugger/GameplayDebuggerCategory_ViewModes.h"

#if WITH_GAMEPLAY_DEBUGGER
	#include "BufferVisualizationData.h"
	#include "Engine/DebugCameraControllerSettings.h"
	#include "Engine/Font.h"
	#include "Engine/GameViewportClient.h"
	#include "EngineUtils.h"
	#include "GameFramework/PlayerController.h"
	#include "LogOpenUnrealUtilities.h"

namespace OUU::Runtime::Private
{
	// Copied from ShowFlags.cpp, but that version is not DLL exported
	const TCHAR* GetViewModeName(EViewModeIndex ViewModeIndex)
	{
		switch (ViewModeIndex)
		{
		case VMI_Unknown: return TEXT("Unknown");
		case VMI_BrushWireframe: return TEXT("BrushWireframe");
		case VMI_Wireframe: return TEXT("Wireframe");
		case VMI_Unlit: return TEXT("Unlit");
		case VMI_Lit: return TEXT("Lit");
		case VMI_Lit_DetailLighting: return TEXT("Lit_DetailLighting");
		case VMI_LightingOnly: return TEXT("LightingOnly");
		case VMI_LightComplexity: return TEXT("LightComplexity");
		case VMI_ShaderComplexity: return TEXT("ShaderComplexity");
		case VMI_QuadOverdraw: return TEXT("QuadOverdraw");
		case VMI_ShaderComplexityWithQuadOverdraw: return TEXT("ShaderComplexityWithQuadOverdraw");
		case VMI_PrimitiveDistanceAccuracy: return TEXT("PrimitiveDistanceAccuracy");
		case VMI_MeshUVDensityAccuracy: return TEXT("MeshUVDensityAccuracy");
		case VMI_MaterialTextureScaleAccuracy: return TEXT("MaterialTextureScaleAccuracy");
		case VMI_RequiredTextureResolution: return TEXT("RequiredTextureResolution");
		case VMI_VirtualTexturePendingMips: return TEXT("VirtualTexturePendingMips");
		case VMI_StationaryLightOverlap: return TEXT("StationaryLightOverlap");
		case VMI_LightmapDensity: return TEXT("LightmapDensity");
		case VMI_LitLightmapDensity: return TEXT("LitLightmapDensity");
		case VMI_ReflectionOverride: return TEXT("ReflectionOverride");
		case VMI_VisualizeBuffer: return TEXT("VisualizeBuffer");
		case VMI_VisualizeNanite: return TEXT("VisualizeNanite");
		case VMI_VisualizeLumen: return TEXT("VisualizeLumen");
		case VMI_VisualizeVirtualShadowMap: return TEXT("VisualizeVirtualShadowMap");
		case VMI_RayTracingDebug: return TEXT("RayTracingDebug");
		case VMI_PathTracing: return TEXT("PathTracing");
		case VMI_CollisionPawn: return TEXT("CollisionPawn");
		case VMI_CollisionVisibility: return TEXT("CollisionVis");
		case VMI_LODColoration: return TEXT("LODColoration");
		case VMI_HLODColoration: return TEXT("HLODColoration");
		}
		return TEXT("");
	}

	// #TODO-OUU replace or remove - Not really required in the code we copy/pasted
	FString GetBufferMaterialName(const FString& InBufferName)
	{
		if (!InBufferName.IsEmpty())
		{
			if (UMaterialInterface* Material = GetBufferVisualizationData().GetMaterial(*InBufferName))
			{
				return Material->GetName();
			}
		}

		return TEXT("");
	}

} // namespace OUU::Runtime::Private

FGameplayDebuggerCategory_ViewModes::FGameplayDebuggerCategory_ViewModes()
{
	BindKeyPress(
		TEXT("Cylce View Mode"),
		EKeys::V.GetFName(),
		FGameplayDebuggerInputModifier::None,
		this,
		&FGameplayDebuggerCategory_ViewModes::CycleViewMode);
	BindKeyPress(
		TEXT("Buffer Overview"),
		EKeys::B.GetFName(),
		FGameplayDebuggerInputModifier::None,
		this,
		&FGameplayDebuggerCategory_ViewModes::ToggleBufferVisualizationOverviewMode);
	BindKeyPress(
		TEXT("Buffer Visualization (Full)"),
		EKeys::Enter.GetFName(),
		FGameplayDebuggerInputModifier::None,
		this,
		&FGameplayDebuggerCategory_ViewModes::ToggleBufferVisualizationFullMode);

	#define BIND_NAVIGATION(Dir)                                                                                       \
		BindKeyPress(                                                                                                  \
			TEXT("Navigate Overview - " PREPROCESSOR_TO_STRING(Dir)),                                                  \
			EKeys::Dir.GetFName(),                                                                                     \
			FGameplayDebuggerInputModifier::None,                                                                      \
			this,                                                                                                      \
			&FGameplayDebuggerCategory_ViewModes::BufferVisualizationMove##Dir)
	BIND_NAVIGATION(Up);
	BIND_NAVIGATION(Down);
	BIND_NAVIGATION(Left);
	BIND_NAVIGATION(Right);
	#undef BIND_NAVIGATION
}

void FGameplayDebuggerCategory_ViewModes::DrawData(
	APlayerController* InOwnerPC,
	FGameplayDebuggerCanvasContext& CanvasContext)
{
	CanvasContext.FontRenderInfo.bEnableShadow = true;
	CanvasContext.Font = GEngine->GetSmallFont();

	PrintKeyBinds(CanvasContext);

	this->OwnerPC = InOwnerPC;
}

void FGameplayDebuggerCategory_ViewModes::CycleViewMode()
{
	if (!OwnerPC.IsValid())
		return;

	if (bEnableBufferVisualization)
	{
		ToggleBufferVisualizationOverviewMode();
	}

	if (UGameViewportClient* GameViewportClient = OwnerPC->GetWorld()->GetGameViewport())
	{
		TArray<EViewModeIndex> DebugViewModes = UDebugCameraControllerSettings::Get()->GetCycleViewModes();

		if (DebugViewModes.Num() == 0)
		{
			UE_LOG(
				LogOpenUnrealUtilities,
				Warning,
				TEXT("Debug camera controller settings must specify at least one view mode for view mode cycling."));
			return;
		}

		int32 CurrViewModeIndex = GameViewportClient->ViewModeIndex;
		int32 CurrIndex = LastViewModeSettingsIndex;

		int32 NextIndex = CurrIndex < DebugViewModes.Num() ? (CurrIndex + 1) % DebugViewModes.Num() : 0;
		int32 NextViewModeIndex = DebugViewModes[NextIndex];

		if (NextViewModeIndex != CurrViewModeIndex)
		{
			FString NextViewModeName = OUU::Runtime::Private::GetViewModeName((EViewModeIndex)NextViewModeIndex);

			if (!NextViewModeName.IsEmpty())
			{
				FString Cmd(TEXT("VIEWMODE "));
				Cmd += NextViewModeName;
				GameViewportClient->ConsoleCommand(Cmd);
			}
			else
			{
				UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("Invalid view mode index %d."), NextViewModeIndex);
			}
		}

		LastViewModeSettingsIndex = NextIndex;
	}
}

TArray<FString> FGameplayDebuggerCategory_ViewModes::GetBufferVisualizationOverviewTargets()
{
	TArray<FString> SelectedBuffers;

	// Get the list of requested buffers from the console
	static IConsoleVariable* CVar =
		IConsoleManager::Get().FindConsoleVariable(TEXT("r.BufferVisualizationOverviewTargets"));
	if (CVar)
	{
		FString SelectedBufferNames = CVar->GetString();

		// Extract each material name from the comma separated string
		while (SelectedBufferNames.Len() && SelectedBuffers.Num() < 16)
		{
			FString Left, Right;

			// Detect last entry in the list
			if (!SelectedBufferNames.Split(TEXT(","), &Left, &Right))
			{
				Left = SelectedBufferNames;
				Right = FString();
			}

			Left.TrimStartInline();
			if (OUU::Runtime::Private::GetBufferMaterialName(*Left).IsEmpty())
			{
				SelectedBuffers.Add(TEXT(""));
			}
			else
			{
				SelectedBuffers.Add(*Left);
			}
			SelectedBufferNames = Right;
		}
	}
	else
	{
		UE_LOG(
			LogOpenUnrealUtilities,
			Warning,
			TEXT("Console variable r.BufferVisualizationOverviewTargets is not found."));
	}
	return SelectedBuffers;
}

void FGameplayDebuggerCategory_ViewModes::ToggleBufferVisualizationOverviewMode()
{
	if (!OwnerPC.IsValid())
		return;

	SetBufferVisualizationFullMode(false);

	if (UGameViewportClient* GameViewportClient = OwnerPC->GetWorld()->GetGameViewport())
	{
		bEnableBufferVisualization = !bEnableBufferVisualization;

		FString Cmd(TEXT("VIEWMODE "));

		if (bEnableBufferVisualization)
		{
			Cmd += OUU::Runtime::Private::GetViewModeName(VMI_VisualizeBuffer);

			TArray<FString> SelectedBuffers = GetBufferVisualizationOverviewTargets();

			if (CurrSelectedBuffer.IsEmpty() || !SelectedBuffers.Contains(CurrSelectedBuffer))
			{
				GetNextBuffer(SelectedBuffers, 1);
			}
		}
		else
		{
			Cmd += OUU::Runtime::Private::GetViewModeName(EViewModeIndex::VMI_Lit);
		}

		GameViewportClient->ConsoleCommand(Cmd);

		SetupBufferVisualizationOverviewInput();
	}
}

void FGameplayDebuggerCategory_ViewModes::GetNextBuffer(int32 Step)
{
	if (bEnableBufferVisualization && !bEnableBufferVisualizationFullMode)
	{
		TArray<FString> OverviewBuffers = GetBufferVisualizationOverviewTargets();
		GetNextBuffer(OverviewBuffers, Step);
	}
}

void FGameplayDebuggerCategory_ViewModes::GetNextBuffer(const TArray<FString>& OverviewBuffers, int32 Step)
{
	if (bEnableBufferVisualization && !bEnableBufferVisualizationFullMode)
	{
		int32 BufferIndex = 0;

		if (!CurrSelectedBuffer.IsEmpty())
		{
			bool bFoundIndex = false;

			for (int32 Index = 0; Index < OverviewBuffers.Num(); Index++)
			{
				if (OverviewBuffers[Index] == CurrSelectedBuffer)
				{
					BufferIndex = Index;
					bFoundIndex = true;
					break;
				}
			}

			if (!bFoundIndex)
			{
				CurrSelectedBuffer.Empty();
			}
		}

		if (CurrSelectedBuffer.IsEmpty())
		{
			for (FString Buffer : OverviewBuffers)
			{
				if (!Buffer.IsEmpty())
				{
					CurrSelectedBuffer = Buffer;
					break;
				}
			}
		}
		else
		{
			int32 Incr = FMath::Abs(Step);
			int32 Min = Incr == 1 ? (BufferIndex / 4) * 4 : BufferIndex % 4;
			int32 Max = Min;
			for (int32 i = 0; i < 3 && Max + Incr < OverviewBuffers.Num(); i++)
			{
				Max += Incr;
			}

			auto Wrap = [&](int32 Index) {
				if (Index < Min)
				{
					Index = Max;
				}
				else if (Index > Max)
				{
					Index = Min;
				}

				return Index;
			};

			int32 NextIndex = Wrap(BufferIndex + Step);

			while (NextIndex != BufferIndex)
			{
				if (!OverviewBuffers[NextIndex].IsEmpty())
				{
					CurrSelectedBuffer = OverviewBuffers[NextIndex];
					break;
				}
				NextIndex = Wrap(NextIndex + Step);
			}
		}
	}
}

void FGameplayDebuggerCategory_ViewModes::BufferVisualizationMoveUp()
{
	GetNextBuffer(-4);
}

void FGameplayDebuggerCategory_ViewModes::BufferVisualizationMoveDown()
{
	GetNextBuffer(4);
}

void FGameplayDebuggerCategory_ViewModes::BufferVisualizationMoveRight()
{
	GetNextBuffer(1);
}

void FGameplayDebuggerCategory_ViewModes::BufferVisualizationMoveLeft()
{
	GetNextBuffer(-1);
}

void FGameplayDebuggerCategory_ViewModes::ToggleBufferVisualizationFullMode()
{
	SetBufferVisualizationFullMode(!bEnableBufferVisualizationFullMode);
}

void FGameplayDebuggerCategory_ViewModes::SetBufferVisualizationFullMode(bool bFullMode)
{
	if (bEnableBufferVisualizationFullMode != bFullMode)
	{
		static IConsoleVariable* ICVar = IConsoleManager::Get().FindConsoleVariable(
			FBufferVisualizationData::GetVisualizationTargetConsoleCommandName());
		if (ICVar)
		{
			bEnableBufferVisualizationFullMode = bFullMode;

			static const FName EmptyName = NAME_None;
			ICVar->Set(bFullMode ? *CurrSelectedBuffer : *EmptyName.ToString(), ECVF_SetByCode);

			SetupBufferVisualizationOverviewInput();
		}
		else
		{
			UE_LOG(
				LogOpenUnrealUtilities,
				Verbose,
				TEXT("Console variable %s does not exist."),
				FBufferVisualizationData::GetVisualizationTargetConsoleCommandName());
		}
	}
}

void FGameplayDebuggerCategory_ViewModes::SetupBufferVisualizationOverviewInput()
{
	// #TODO-OUU
	bIsBufferVisualizationInputSetup = true;
}

#endif
