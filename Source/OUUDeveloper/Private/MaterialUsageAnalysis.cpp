// Copyright (c) 2023 Jonas Reich & Contributors

#include "CoreMinimal.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/LevelStreaming.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/HUD.h"
#include "LogOpenUnrealUtilities.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CanvasGraphPlottingUtils.h"
#include "OUUWorldStatsOverlay.h"
#include "Templates/CastObjectRange.h"
#include "Templates/RingAggregator.h"
#include "Templates/StringUtils.h"
#include "Tickable.h"
#include "UnrealClient.h"

constexpr int32 NumFramesForBuffer = 100;
constexpr float UpdateInterval = 0.1f;

#define MATERIAL_ANALYSIS_BASE_CVAR "ouu.Debug.MaterialUsageAnalysis"

#define DECLARE_CVAR(Type, CVar, Command, DefaultValue, HelpText)                                                      \
	TAutoConsoleVariable<Type> CVar{TEXT(MATERIAL_ANALYSIS_BASE_CVAR Command), DefaultValue, TEXT(HelpText)};          \
	FString CVar##_Name = TEXT(Command);

DECLARE_CVAR(
	int32,
	CVarMinInstances,
	".MinInstances",
	3,
	"Minimum number of static meshes that will be considered to be instanced");

DECLARE_CVAR(
	bool,
	CVarOnlyRecentlyRendered,
	".OnlyAnalyzeRecentlyRendered",
	true,
	"If true, only recently rendered prims are included in analysis (to give more relevant results)");

DECLARE_CVAR(
	bool,
	CVarAllowMovableInstances,
	".AllowMovableInstances",
	false,
	"Whether movable static meshes should be considered to be converted to instanced static meshes. When turned "
	"off, it's assumed they cannot be converted.");

DECLARE_CVAR(
	bool,
	CVarExcludeVirtualTextureOnlyMeshes,
	".ExcludeVirtualTextureOnlyMeshes",
	true,
	"Exclude meshes that are configured to only render into the virtual texture");

DECLARE_CVAR(bool, CVarUseLogarithmicYAxis, ".LogYAxis", false, "Draw the on-screen graphs with logarithmic Y axis");

#undef DECLARE_CVAR

using MeshMaterialCombinationType = TTuple<UObject*, FString>;

struct FMeshStats
{
	// material objects that are serialized as string in key
	TArray<UMaterialInterface*> MaterialObjects;

	int32 NumStaticMeshComponentsNow = 0;
	int32 NumStaticMeshInstances_Now = 0;
	int32 NumStaticMeshInstances_Possible = 0;
	int32 NumStaticMeshInstances_Max = 0;

	int32 NumSkinnedMeshComponents = 0;
};

struct FMaterialAnalysisResults
{
	TMap<MeshMaterialCombinationType, FMeshStats> MeshStatsByCombo;
	int32 NumPrimitivesWithoutMesh = 0;
	int32 NumUnrecognizedPrimitivesWithMesh = 0;
	int32 NumIgnoredPrimitivesNotRendered = 0;
	TMap<UClass*, int32> UnsupportedPrimCounts;

	FMeshStats MeshStatsSum;
	int32 NumStaticMeshComponents_Best = 0;

	int32 PotentialComponentSave_ByInstancing;
	float PotentialComponentSave_ByInstancing_Percentage;

	int32 DrawCalls_Current = 0;
	int32 DrawCalls_Best = 0;

	int32 NumUniqueMaterials = 0;
};

UObject* GetMeshFromPrimitiveComponent(const UPrimitiveComponent* PrimitiveComponent)
{
	if (auto* StaticMeshComp = Cast<UStaticMeshComponent>(PrimitiveComponent))
	{
		return StaticMeshComp->GetStaticMesh();
	}
	if (auto* SkinnedMeshComponent = Cast<USkinnedMeshComponent>(PrimitiveComponent))
	{
		return SkinnedMeshComponent->GetSkinnedAsset();
	}
	return nullptr;
}

FMaterialAnalysisResults AnalyzeMaterialUsage(UWorld* TargetWorld)
{
	const bool bOnlyRecentlyRendered = CVarOnlyRecentlyRendered.GetValueOnAnyThread();
	const bool bExcludeVTOnlyMeshes = CVarExcludeVirtualTextureOnlyMeshes.GetValueOnAnyThread();

	FMaterialAnalysisResults Results;
	TSet<UMaterialInterface*> UniqueMaterials;
	for (const auto* Actor : TActorRange<AActor>(TargetWorld))
	{
		Actor->ForEachComponent<UPrimitiveComponent>(false, [&](const UPrimitiveComponent* PrimitiveComponent) {
			auto* Mesh = GetMeshFromPrimitiveComponent(PrimitiveComponent);
			if (!Mesh)
			{
				Results.NumPrimitivesWithoutMesh += 1;
				Results.UnsupportedPrimCounts.FindOrAdd(PrimitiveComponent->GetClass(), 0) += 1;
				return;
			}
			// Exclude b/c it wasn't recently rendered?
			if (bOnlyRecentlyRendered
				&& (!PrimitiveComponent->WasRecentlyRendered() || !PrimitiveComponent->IsVisible()
					|| PrimitiveComponent->bHiddenInGame))
			{
				Results.NumIgnoredPrimitivesNotRendered += 1;
				return;
			}
			// Exclude b/c of Virtual Texture?
			if (bExcludeVTOnlyMeshes
				&& PrimitiveComponent->GetVirtualTextureRenderPassType() != ERuntimeVirtualTextureMainPassType::Always
				&& PrimitiveComponent->GetRuntimeVirtualTextures().Num() > 0)
			{
				Results.NumIgnoredPrimitivesNotRendered += 1;
				return;
			}

			TArray<UMaterialInterface*> Materials;
			constexpr bool bGetDebugMaterials = false;
			PrimitiveComponent->GetUsedMaterials(OUT Materials, bGetDebugMaterials);
			auto& Stats =
				Results.MeshStatsByCombo.FindOrAdd(MeshMaterialCombinationType{Mesh, ArrayToString(Materials)});
			if (Stats.MaterialObjects.Num() == 0)
			{
				// Move materials into stats. Only access the material member after this!
				Stats.MaterialObjects = MoveTemp(Materials);
				Stats.MaterialObjects.RemoveAll([](auto* M) { return !IsValid(M); });
				for (auto* Material : Stats.MaterialObjects)
				{
					UniqueMaterials.Add(Material);
				}
			}

			// Assume 1 draw call per material per mesh section per mesh component
			// -> Realistically it's (2 + 1 * lights) + other
			// with "other" being any other visual system that adds draw calls, e.g. rendering to stencil buffer, etc.
			// Ignore any optimizations by disabling shadows and/or setting fully translucent material for now.
			Results.DrawCalls_Current += Stats.MaterialObjects.Num();

			if (auto* InstancedStaticMeshComponent = Cast<UInstancedStaticMeshComponent>(PrimitiveComponent))
			{
				Stats.NumStaticMeshComponentsNow += 1;
				const int32 NumInstances = InstancedStaticMeshComponent->GetInstanceCount();
				Stats.NumStaticMeshInstances_Now += NumInstances;
				Stats.NumStaticMeshInstances_Possible += NumInstances;
				Stats.NumStaticMeshInstances_Max += NumInstances;
			}
			else if (PrimitiveComponent->IsA<UStaticMeshComponent>())
			{
				Stats.NumStaticMeshComponentsNow += 1;
				if (PrimitiveComponent->Mobility == EComponentMobility::Static
					|| CVarAllowMovableInstances.GetValueOnAnyThread())
				{
					Stats.NumStaticMeshInstances_Possible += 1;
				}
				Stats.NumStaticMeshInstances_Max += 1;
			}
			else if (PrimitiveComponent->IsA<USkinnedMeshComponent>())
			{
				Stats.NumSkinnedMeshComponents += 1;
			}
			else
			{
				Results.NumUnrecognizedPrimitivesWithMesh++;
				Results.UnsupportedPrimCounts.FindOrAdd(PrimitiveComponent->GetClass(), 0) += 1;
			}
		});
	}

	for (const auto& Entry : Results.MeshStatsByCombo)
	{
		const FMeshStats& Stats = Entry.Value;
		Results.MeshStatsSum.NumStaticMeshComponentsNow += Stats.NumStaticMeshComponentsNow;
		Results.MeshStatsSum.NumStaticMeshInstances_Now += Stats.NumStaticMeshInstances_Now;
		Results.MeshStatsSum.NumStaticMeshInstances_Possible += Stats.NumStaticMeshInstances_Possible;
		Results.MeshStatsSum.NumStaticMeshInstances_Max += Stats.NumStaticMeshInstances_Max;
		const int32 NumComponents_Best = Stats.NumStaticMeshInstances_Possible > CVarMinInstances.GetValueOnAnyThread()
			? 1
			: Stats.NumStaticMeshComponentsNow;
		Results.NumStaticMeshComponents_Best += NumComponents_Best;
		Results.DrawCalls_Best += Stats.MaterialObjects.Num() * NumComponents_Best;
		Results.MeshStatsSum.NumSkinnedMeshComponents += Stats.NumSkinnedMeshComponents;
	}

	Results.PotentialComponentSave_ByInstancing =
		Results.NumStaticMeshComponents_Best - Results.MeshStatsSum.NumStaticMeshComponentsNow;
	Results.PotentialComponentSave_ByInstancing_Percentage =
		(static_cast<float>(Results.PotentialComponentSave_ByInstancing)
		 / static_cast<float>(Results.MeshStatsSum.NumStaticMeshComponentsNow))
		* 100.f;

	Results.NumUniqueMaterials = UniqueMaterials.Num();

	return Results;
}

void DumpMaterialAnalysis(UWorld* TargetWorld)
{
	auto Results = AnalyzeMaterialUsage(TargetWorld);

	TArray<FString> LoadedLevelsStrings;
	for (auto& Level : TargetWorld->GetStreamingLevels())
	{
		if (Level->IsLevelLoaded())
		{
			auto LongPackageName = Level->GetWorldAssetPackageName();
			FString PackageRoot, PackagePath, PackageName;
			FPackageName::SplitLongPackageName(LongPackageName, OUT PackageRoot, OUT PackagePath, OUT PackageName);
			PackageName.RemoveFromStart(TEXT("UEDPIE_0_"));
			LoadedLevelsStrings.Add(PackageName);
		}
	}

	FString AnalysisLogString;
#define UE_ANALYSIS_LOG(Format, ...) AnalysisLogString += FString::Printf(TEXT(Format "\n"), ##__VA_ARGS__)
#define UE_ANALYSIS_LOG_CVAR(CVar)	 UE_ANALYSIS_LOG("\t%s: %s", *CVar##_Name, *LexToString(CVar.GetValueOnAnyThread()));

	UE_ANALYSIS_LOG("---------------------------------------------------------------");
	UE_ANALYSIS_LOG("Material usage analysis completed. Summary:");
	UE_ANALYSIS_LOG("---------------------------------------------------------------");
	UE_ANALYSIS_LOG("Settings:");
	UE_ANALYSIS_LOG_CVAR(CVarMinInstances);
	UE_ANALYSIS_LOG_CVAR(CVarOnlyRecentlyRendered);
	UE_ANALYSIS_LOG_CVAR(CVarAllowMovableInstances);
	UE_ANALYSIS_LOG_CVAR(CVarExcludeVirtualTextureOnlyMeshes);
	UE_ANALYSIS_LOG("---------------------------------------------------------------");
	UE_ANALYSIS_LOG("World: %s", *TargetWorld->GetName());
	UE_ANALYSIS_LOG("Loaded streaming levels: %s", *FString::Join(LoadedLevelsStrings, TEXT(", ")));
	UE_ANALYSIS_LOG("---------------------------------------------------------------");
	UE_ANALYSIS_LOG("Unique materials/mesh combinations: %i", Results.MeshStatsByCombo.Num());

	UE_ANALYSIS_LOG("---------------------------------------------------------------");
	UE_ANALYSIS_LOG("SM components: %i", Results.MeshStatsSum.NumStaticMeshComponentsNow);
	UE_ANALYSIS_LOG("SM instances (now): %i", Results.MeshStatsSum.NumStaticMeshInstances_Now);
	UE_ANALYSIS_LOG("SM instances (possible): %i", Results.MeshStatsSum.NumStaticMeshInstances_Possible);
	UE_ANALYSIS_LOG("SM instances (max - including disqualified): %i", Results.MeshStatsSum.NumStaticMeshInstances_Max);
	UE_ANALYSIS_LOG(
		"SM instances (disqualified): %i",
		Results.MeshStatsSum.NumStaticMeshInstances_Max - Results.MeshStatsSum.NumStaticMeshInstances_Possible);
	UE_ANALYSIS_LOG(
		"SM Components (best case): %i (potential save: %i / %.2f%%)",
		Results.NumStaticMeshComponents_Best,
		Results.PotentialComponentSave_ByInstancing,
		Results.PotentialComponentSave_ByInstancing_Percentage);
	UE_ANALYSIS_LOG("---------------------------------------------------------------");
	UE_ANALYSIS_LOG("Ignored prims (not rendered / VT only): %i", Results.NumIgnoredPrimitivesNotRendered);
	UE_ANALYSIS_LOG("Skinned meshes: %i", Results.MeshStatsSum.NumSkinnedMeshComponents);
	UE_ANALYSIS_LOG("Primitives w/o mesh: %i", Results.NumPrimitivesWithoutMesh);
	UE_ANALYSIS_LOG("Unrecognized Prims w/ mesh: %i", Results.NumUnrecognizedPrimitivesWithMesh);
	UE_ANALYSIS_LOG("Not-fully supported primitive component classes: %s", *MapToString(Results.UnsupportedPrimCounts));
	UE_ANALYSIS_LOG("---------------------------------------------------------------");
#undef UE_ANALYSIS_LOG
#undef UE_ANALYSIS_LOG_CVAR

	UE_LOG(LogOpenUnrealUtilities, Log, TEXT(" \n%s"), *AnalysisLogString);
}

class FMaterialAnalysisOverlay : public OUU::Developer::TWorldStatsOverlay<FMaterialAnalysisOverlay>
{
public:
	FMaterialAnalysisOverlay() : TWorldStatsOverlay(::UpdateInterval)
	{
		auto& ComponentStats = GraphStats.AddDefaulted_GetRef();
		ComponentStats.Name = TEXT("static mesh components");
		ComponentStats.DataSeries.Add({Buffer_ComponentsNow, FColorList::Red, "now"});
		ComponentStats.DataSeries.Add({Buffer_ComponentsBest, FColorList::Green, "best"});

		auto& DrawCallsStats = GraphStats.AddDefaulted_GetRef();
		DrawCallsStats.Name = TEXT("static mesh draw calls");
		DrawCallsStats.DataSeries.Add({Buffer_DrawCallsNow, FColorList::Red, "now"});
		DrawCallsStats.DataSeries.Add({Buffer_DrawCallsBest, FColorList::Green, "best"});
		DrawCallsStats.DataSeries.Add({Buffer_Materials, FColorList::LightBlue, "materials"});
		DrawCallsStats.DataSeries.Add({Buffer_MaterialCombinations, FColorList::Yellow, "mat combos"});

		auto& InstanceStats = GraphStats.AddDefaulted_GetRef();
		InstanceStats.Name = TEXT("mesh instances");
		InstanceStats.DataSeries.Add({Buffer_NumStaticMeshInstances_Max, FColorList::Violet, "max"});
		InstanceStats.DataSeries.Add({Buffer_NumStaticMeshInstances_Now, FColorList::Red, "now"});
		InstanceStats.DataSeries.Add({Buffer_NumStaticMeshInstances_Possible, FColorList::Green, "best"});
	}

private:
	// Component stats
	TCircularAggregator<float> Buffer_ComponentsNow{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_ComponentsBest{NumFramesForBuffer};

	// Draw calls
	TCircularAggregator<float> Buffer_DrawCallsNow{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_DrawCallsBest{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_Materials{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_MaterialCombinations{NumFramesForBuffer};

	// Instances
	TCircularAggregator<float> Buffer_NumStaticMeshInstances_Max{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_NumStaticMeshInstances_Now{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_NumStaticMeshInstances_Possible{NumFramesForBuffer};

	// - TWorldStatsOverlay
	void TickStats(UWorld* TargetWorld) override
	{
		const bool bUseLogarithmicYAxis = CVarUseLogarithmicYAxis.GetValueOnAnyThread();
		for (auto& Entry : GraphStats)
		{
			Entry.bUseLogarithmicAxis = bUseLogarithmicYAxis;
		}

		const auto Results = AnalyzeMaterialUsage(TargetWorld);

		// Components
		Buffer_ComponentsNow.Add(Results.MeshStatsSum.NumStaticMeshComponentsNow);
		Buffer_ComponentsBest.Add(Results.NumStaticMeshComponents_Best);

		// Draw calls
		Buffer_DrawCallsNow.Add(Results.DrawCalls_Current);
		Buffer_DrawCallsBest.Add(Results.DrawCalls_Best);
		Buffer_Materials.Add(Results.NumUniqueMaterials);
		Buffer_MaterialCombinations.Add(Results.MeshStatsByCombo.Num());

		// Instances
		Buffer_NumStaticMeshInstances_Max.Add(Results.MeshStatsSum.NumStaticMeshInstances_Max);
		Buffer_NumStaticMeshInstances_Now.Add(Results.MeshStatsSum.NumStaticMeshInstances_Now);
		Buffer_NumStaticMeshInstances_Possible.Add(Results.MeshStatsSum.NumStaticMeshInstances_Possible);
	}
};

DEFINE_OUU_WORLD_STAT_OVERLAY(
	FMaterialAnalysisOverlay,
	MATERIAL_ANALYSIS_BASE_CVAR,
	"Toggle displaying stats of static meshes and their materials in the current world as on-screen graphs")

static FAutoConsoleCommand AnalyzeMaterialUsage_Command(
	TEXT(MATERIAL_ANALYSIS_BASE_CVAR ".Dump"),
	TEXT("Write stats of static meshes and their materials in the current world to the log"),
	FConsoleCommandDelegate::CreateStatic([]() { DumpMaterialAnalysis(FMaterialAnalysisOverlay::GetStatsWorld()); }));
