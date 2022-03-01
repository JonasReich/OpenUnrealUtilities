// Copyright (c) 2022 Jonas Reich

#include "CoreMinimal.h"

#include "CanvasItem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Containers/CircularBuffer.h"
#include "Engine/Canvas.h"
#include "Engine/LevelStreaming.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/HUD.h"
#include "LogOpenUnrealUtilities.h"
#include "Misc/CanvasGraphPlottingUtils.h"
#include "Templates/CastObjectRange.h"
#include "Templates/RingAggregator.h"
#include "Templates/StringUtils.h"
#include "Tickable.h"

#if WITH_EDITOR
	#include "Editor.h"
#endif

const int32 NumFramesForBuffer = 100;
const float UpdateInterval = 0.1f;

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
		return SkinnedMeshComponent->SkeletalMesh;
	}
	return nullptr;
}

UWorld* GetTargetWorld()
{
	auto* TargetWorld = GEngine->GetCurrentPlayWorld();
#if WITH_EDITOR
	if (!TargetWorld)
	{
		TargetWorld = GEditor->GetEditorWorldContext().World();
	}
#endif
	UE_CLOG(
		!IsValid(TargetWorld),
		LogOpenUnrealUtilities,
		Error,
		TEXT("Did not find any world to perform material analysis"));
	return TargetWorld;
}

FMaterialAnalysisResults AnalyzeMaterialUsage(UWorld* TargetWorld)
{
	const bool bOnlyRecentlyRendered = CVarOnlyRecentlyRendered.GetValueOnAnyThread();
	const bool bExcludeVTOnlyMeshes = CVarExcludeVirtualTextureOnlyMeshes.GetValueOnAnyThread();

	FMaterialAnalysisResults Results;
	TSet<UMaterialInterface*> UniqueMaterials;
	for (auto* Actor : TActorRange<AActor>(TargetWorld))
	{
		Actor->ForEachComponent<UPrimitiveComponent>(false, [&](const UPrimitiveComponent* PrimitiveComponent) {
			auto* Mesh = GetMeshFromPrimitiveComponent(PrimitiveComponent);
			if (!Mesh)
			{
				Results.NumPrimitivesWithoutMesh += 1;
				Results.UnsupportedPrimCounts.FindOrAdd(PrimitiveComponent->GetClass(), 0) += 1;
				return;
			}
			if (
				// Exclude b/c it wasn't recently rendered?
				(bOnlyRecentlyRendered && (!PrimitiveComponent->WasRecentlyRendered())
				 || !PrimitiveComponent->IsVisible() || PrimitiveComponent->bHiddenInGame)
				// Exclude b/c of Virtual Texture?
				|| (bExcludeVTOnlyMeshes
					&& PrimitiveComponent->GetVirtualTextureRenderPassType()
						!= ERuntimeVirtualTextureMainPassType::Always
					&& PrimitiveComponent->GetRuntimeVirtualTextures().Num() > 0))
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
			PackageName.RemoveFromStart("UEDPIE_0_");
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

class FMaterialAnalysisTickHelper : public FTickableGameObject
{
private:
	static TUniquePtr<FMaterialAnalysisTickHelper> GTickHelper;

public:
	FMaterialAnalysisTickHelper()
	{
		ComponentStats.Add({Buffer_ComponentsNow, FColorList::Red, "now"});
		ComponentStats.Add({Buffer_ComponentsBest, FColorList::Green, "best"});

		DrawCallsStats.Add({Buffer_DrawCallsNow, FColorList::Red, "now"});
		DrawCallsStats.Add({Buffer_DrawCallsBest, FColorList::Green, "best"});
		DrawCallsStats.Add({Buffer_Materials, FColorList::LightBlue, "materials"});
		DrawCallsStats.Add({Buffer_MaterialCombinations, FColorList::Yellow, "mat combos"});

		InstanceStats.Add({Buffer_NumStaticMeshInstances_Max, FColorList::Violet, "max"});
		InstanceStats.Add({Buffer_NumStaticMeshInstances_Now, FColorList::Red, "now"});
		InstanceStats.Add({Buffer_NumStaticMeshInstances_Possible, FColorList::Green, "best"});
	}

	static void Toggle()
	{
		if (GTickHelper.IsValid())
		{
			AHUD::OnHUDPostRender.RemoveAll(GTickHelper.Get());
			GTickHelper.Reset();
		}
		else
		{
			GTickHelper = MakeUnique<FMaterialAnalysisTickHelper>();
			AHUD::OnHUDPostRender.AddRaw(GTickHelper.Get(), &FMaterialAnalysisTickHelper::OnShowDebugInfo);
		}
	}

private:
	float LastUpdateTime = 0.f;
	float AccumulatedTime = 0.f;

	// Component stats
	TCircularAggregator<float> Buffer_ComponentsNow{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_ComponentsBest{NumFramesForBuffer};
	TArray<CanvasGraphPlottingUtils::FGraphStatData> ComponentStats;
	float MaxNumComponents = 0.f;

	// Draw calls
	TCircularAggregator<float> Buffer_DrawCallsNow{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_DrawCallsBest{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_Materials{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_MaterialCombinations{NumFramesForBuffer};
	TArray<CanvasGraphPlottingUtils::FGraphStatData> DrawCallsStats;
	float MaxDrawCalls = 0.f;

	// Instances
	TCircularAggregator<float> Buffer_NumStaticMeshInstances_Max{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_NumStaticMeshInstances_Now{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_NumStaticMeshInstances_Possible{NumFramesForBuffer};
	TArray<CanvasGraphPlottingUtils::FGraphStatData> InstanceStats;
	float MaxInstances = 0.f;

	// - FTickableGameObject
	virtual void Tick(float DeltaTime) override
	{
		AccumulatedTime += DeltaTime;
		if (AccumulatedTime > UpdateInterval)
		{
			auto* TargetWorld = GetTargetWorld();
			if (!TargetWorld)
				return;

			auto Results = AnalyzeMaterialUsage(TargetWorld);

			auto UpdateMax = [](auto& Stats, auto& MaxValue) {
				MaxValue = 100.f;
				for (auto& Stat : Stats)
				{
					MaxValue = FMath::Max(
						MaxValue,
						static_cast<const TCircularAggregator<float>*>(Stat.ValueAggregator.ValueContainer)->Max());
				}
			};

			// Components
			Buffer_ComponentsNow.Add(Results.MeshStatsSum.NumStaticMeshComponentsNow);
			Buffer_ComponentsBest.Add(Results.NumStaticMeshComponents_Best);

			UpdateMax(ComponentStats, MaxNumComponents);

			// Draw calls
			Buffer_DrawCallsNow.Add(Results.DrawCalls_Current);
			Buffer_DrawCallsBest.Add(Results.DrawCalls_Best);
			Buffer_Materials.Add(Results.NumUniqueMaterials);
			Buffer_MaterialCombinations.Add(Results.MeshStatsByCombo.Num());

			UpdateMax(DrawCallsStats, MaxDrawCalls);

			// Instances
			Buffer_NumStaticMeshInstances_Max.Add(Results.MeshStatsSum.NumStaticMeshInstances_Max);
			Buffer_NumStaticMeshInstances_Now.Add(Results.MeshStatsSum.NumStaticMeshInstances_Now);
			Buffer_NumStaticMeshInstances_Possible.Add(Results.MeshStatsSum.NumStaticMeshInstances_Possible);

			UpdateMax(InstanceStats, MaxInstances);
		}
		while (AccumulatedTime > UpdateInterval)
		{
			AccumulatedTime -= UpdateInterval;
		}
	}

	void OnShowDebugInfo(AHUD* HUD, UCanvas* InCanvas)
	{
		const float GraphBottomYPos =
			InCanvas->Canvas->GetRenderTarget()->GetSizeXY().Y / InCanvas->GetDPIScale() - 50.0f;

		const bool bUseLogarithmicYAxis = CVarUseLogarithmicYAxis.GetValueOnAnyThread();

		CanvasGraphPlottingUtils::DrawCanvasGraph(
			InCanvas->Canvas,
			80.0f + 450.0f + 350.f * 0.f,
			GraphBottomYPos,
			ComponentStats,
			"static mesh components",
			MaxNumComponents,
			bUseLogarithmicYAxis);
		CanvasGraphPlottingUtils::DrawCanvasGraph(
			InCanvas->Canvas,
			80.0f + 450.0f + 350.f * 1.f,
			GraphBottomYPos,
			DrawCallsStats,
			"static mesh draw calls",
			MaxDrawCalls,
			bUseLogarithmicYAxis);
		CanvasGraphPlottingUtils::DrawCanvasGraph(
			InCanvas->Canvas,
			80.0f + 450.0f + 350.f * 2.f,
			GraphBottomYPos,
			InstanceStats,
			"mesh instances",
			MaxInstances,
			bUseLogarithmicYAxis);
	}

	virtual TStatId GetStatId() const override { return TStatId(); }

	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }

	virtual UWorld* GetTickableGameObjectWorld() const override { return GetTargetWorld(); }
	// --
};

TUniquePtr<FMaterialAnalysisTickHelper> FMaterialAnalysisTickHelper::GTickHelper;

static FAutoConsoleCommand AnalyzeMaterialUsage_Command(
	TEXT(MATERIAL_ANALYSIS_BASE_CVAR ".Dump"),
	TEXT("Write stats of static meshes and their materials in the current world to the log"),
	FConsoleCommandDelegate::CreateStatic([]() { DumpMaterialAnalysis(GetTargetWorld()); }));

static FAutoConsoleCommand StartTickAnalyzeMaterialUsage_Command(
	TEXT(MATERIAL_ANALYSIS_BASE_CVAR),
	TEXT("Toggle displaying stats of static meshes and their materials in the current world as on-screen graphs"),
	FConsoleCommandDelegate::CreateStatic([]() { FMaterialAnalysisTickHelper::Toggle(); }));
