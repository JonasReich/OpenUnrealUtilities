// Copyright (c) 2023 Jonas Reich & Contributors

#include "CoreMinimal.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/LevelStreaming.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "LogOpenUnrealUtilities.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CanvasGraphPlottingUtils.h"
#include "OUUWorldStatsOverlay.h"
#include "PhysicsEngine/BodySetup.h"
#include "Templates/CastObjectRange.h"
#include "Templates/RingAggregator.h"
#include "Templates/StringUtils.h"
#include "Tickable.h"
#include "UnrealClient.h"

constexpr int32 NumFramesForBuffer = 100;
constexpr float UpdateInterval = 0.1f;

#define STATIC_MESH_ANALYSIS_BASE_CVAR "ouu.Debug.StaticMeshAnalysis"

#define DECLARE_CVAR(Type, CVar, Command, DefaultValue, HelpText)                                                      \
	TAutoConsoleVariable<Type> CVar{TEXT(STATIC_MESH_ANALYSIS_BASE_CVAR Command), DefaultValue, TEXT(HelpText)};       \
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
	CVarExcludeEditorOnly,
	".ExcludeEditorOnly",
	true,
	"If true, exclude any meshes that are on editor only actors or components");

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
	"Exclude meshes that are configured to only render into the virtual texture. This does not influence the collision "
	"analysis");

DECLARE_CVAR(bool, CVarUseLogarithmicYAxis, ".LogYAxis", false, "Draw the on-screen graphs with logarithmic Y axis");

#undef DECLARE_CVAR

using MeshCollisionMaterialCombinationType = TTuple<UObject*, ECollisionEnabled::Type, FString>;

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

struct FStaticMeshAnalysisResults
{
	TMap<MeshCollisionMaterialCombinationType, FMeshStats> MeshStatsByCombo;
	int32 NumPrimitivesWithoutMesh = 0;
	int32 NumIgnoredPrimitives = 0;
	TMap<UClass*, int32> UnsupportedPrimCounts;

	FMeshStats MeshStatsSum;
	int32 NumStaticMeshComponents_Best = 0;

	int32 PotentialComponentSave_ByInstancing;
	float PotentialComponentSave_ByInstancing_Percentage;

	int32 DrawCalls_Current = 0;
	int32 DrawCalls_Best = 0;

	int32 NumUniqueMaterials = 0;

	int32 TotalShapeCount = 0;
	int32 TotalExpensiveShapeCount = 0;
	int32 NumStaticMeshComponentsWithPhysics = 0;
	int32 MaxShapeCount = 0;
	int32 MaxExpensiveShapeCount = 0;
	FString MaxExpensiveShapeOwner;
	TArray<FString> CustomCollisionMeshPaths;
	TMap<FName, int32> CollisionProfileCounts;
	int32 TotalNumPhysicsEnabled_Query = 0;
	int32 TotalNumPhysicsEnabled_Physics = 0;
	int32 TotalNumPhysicsEnabled_Probe = 0;

	int32 NumShapes_Small = 0, NumShapes_Medium = 0, NumShapes_Large = 0;
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

FStaticMeshAnalysisResults AnalyzeMaterialUsage(UWorld* TargetWorld)
{
	const bool bOnlyRecentlyRendered = CVarOnlyRecentlyRendered.GetValueOnAnyThread();
	const bool bExcludeVTOnlyMeshes = CVarExcludeVirtualTextureOnlyMeshes.GetValueOnAnyThread();
	const bool bExcludeEditorOnlyObjects = CVarExcludeEditorOnly.GetValueOnAnyThread();

	FStaticMeshAnalysisResults Results;
	TSet<UMaterialInterface*> UniqueMaterials;
	for (const auto* Actor : TActorRange<AActor>(TargetWorld))
	{
		if (bExcludeEditorOnlyObjects && Actor->IsEditorOnly())
		{
			continue;
		}

		Actor->ForEachComponent<UPrimitiveComponent>(false, [&](const UPrimitiveComponent* PrimitiveComponent) {
			if (bExcludeEditorOnlyObjects && PrimitiveComponent->IsEditorOnly())
			{
				return;
			}

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
				Results.NumIgnoredPrimitives += 1;
				return;
			}

			if (PrimitiveComponent->IsCollisionEnabled())
			{
				bool bQuery, bPhysics, bProbe;
				CollisionEnabledToFlags(PrimitiveComponent->GetCollisionEnabled(), bQuery, bPhysics, bProbe);
				if (bQuery)
				{
					++Results.TotalNumPhysicsEnabled_Query;
				}
				if (bPhysics)
				{
					++Results.TotalNumPhysicsEnabled_Physics;
				}
				if (bProbe)
				{
					++Results.TotalNumPhysicsEnabled_Probe;
				}

				if (PrimitiveComponent->GetCollisionProfileName() == TEXT("Custom"))
				{
					if (Results.CustomCollisionMeshPaths.Num() < 10)
					{
						Results.CustomCollisionMeshPaths.Add(FString::Printf(
							TEXT("%s on %s"),
							*Mesh->GetPathName(),
							*PrimitiveComponent->GetPathName()));
					}
				}

				Results.CollisionProfileCounts.FindOrAdd(PrimitiveComponent->GetCollisionProfileName(), 0) += 1;

				// always get the body instance / physics stats
				if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(Mesh))
				{
					if (auto* BodySetup = StaticMesh->GetBodySetup())
					{
						static const TSet<EAggCollisionShape::Type> AllShapeTypes{
							// "cheap" collision
							EAggCollisionShape::Sphere,
							EAggCollisionShape::Box,
							EAggCollisionShape::Sphyl,
							EAggCollisionShape::TaperedCapsule,

							// "expensive" collision
							EAggCollisionShape::Convex,
							EAggCollisionShape::LevelSet,

							// types not fully supported yet in 5.7
							// EAggCollisionShape::SkinnedLevelSet
							// EAggCollisionShape::MLLevelSet
							// EAggCollisionShape::SkinnedTriangleMesh
						};
						static const TSet<EAggCollisionShape::Type> ExpensiveShapeTypes{
							EAggCollisionShape::Convex,
							EAggCollisionShape::LevelSet};

						int32 TotalShapeCount = 0;
						int32 ExpensiveShapeCount = 0;
						int32 TotalExpensiveShapeCount = 0;

						for (auto& Element : AllShapeTypes)
						{
							const int32 Count = BodySetup->AggGeom.GetElementCount(Element);
							TotalShapeCount += Count;

							if (ExpensiveShapeTypes.Contains(Element))
							{
								ExpensiveShapeCount += Count;
							}
						}

						auto AABB = BodySetup->AggGeom.CalcAABB(FTransform{});
						const float Size = AABB.GetSize().GetAbsMax();
						if (Size < 100.f)
						{
							Results.NumShapes_Small += 1;
						}
						else if (Size < 500.f)
						{
							Results.NumShapes_Medium += 1;
						}
						else
						{
							Results.NumShapes_Large += 1;
						}

						++Results.NumStaticMeshComponentsWithPhysics;
						Results.TotalShapeCount += TotalShapeCount;
						Results.TotalExpensiveShapeCount += TotalExpensiveShapeCount;
						Results.MaxShapeCount = FMath::Max(Results.MaxShapeCount, TotalShapeCount);
						if (ExpensiveShapeCount > Results.MaxExpensiveShapeCount)
						{
							Results.MaxExpensiveShapeCount = ExpensiveShapeCount;
							Results.MaxExpensiveShapeOwner = PrimitiveComponent->GetPathName();
						}
					}
				}
			}

			// Exclude b/c of Virtual Texture?
			if (bExcludeVTOnlyMeshes
				&& PrimitiveComponent->GetVirtualTextureRenderPassType() != ERuntimeVirtualTextureMainPassType::Always
				&& PrimitiveComponent->GetRuntimeVirtualTextures().Num() > 0)
			{
				Results.NumIgnoredPrimitives += 1;
				return;
			}

			TArray<UMaterialInterface*> Materials;
			constexpr bool bGetDebugMaterials = false;
			PrimitiveComponent->GetUsedMaterials(OUT Materials, bGetDebugMaterials);
			auto& Stats = Results.MeshStatsByCombo.FindOrAdd(MeshCollisionMaterialCombinationType{
				Mesh,
				PrimitiveComponent->GetCollisionEnabled(),
				ArrayToString(Materials)});
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
	UE_ANALYSIS_LOG("Static mesh analysis completed. Summary:");
	UE_ANALYSIS_LOG("---------------------------------------------------------------");
	UE_ANALYSIS_LOG("Settings:");
	UE_ANALYSIS_LOG_CVAR(CVarMinInstances);
	UE_ANALYSIS_LOG_CVAR(CVarOnlyRecentlyRendered);
	UE_ANALYSIS_LOG_CVAR(CVarExcludeEditorOnly);
	UE_ANALYSIS_LOG_CVAR(CVarAllowMovableInstances);
	UE_ANALYSIS_LOG_CVAR(CVarExcludeVirtualTextureOnlyMeshes);
	UE_ANALYSIS_LOG("---------------------------------------------------------------");
	UE_ANALYSIS_LOG("World: %s", *TargetWorld->GetName());
	UE_ANALYSIS_LOG("Loaded streaming levels: %s", *FString::Join(LoadedLevelsStrings, TEXT(", ")));
	UE_ANALYSIS_LOG("---------------------------------------------------------------");
	UE_ANALYSIS_LOG("Unique mesh/collision/material combinations: %i", Results.MeshStatsByCombo.Num());

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
	UE_ANALYSIS_LOG("Ignored prims (editor only / not rendered / VT only): %i", Results.NumIgnoredPrimitives);
	UE_ANALYSIS_LOG("Skinned meshes: %i", Results.MeshStatsSum.NumSkinnedMeshComponents);
	UE_ANALYSIS_LOG("Primitives w/o mesh: %i", Results.NumPrimitivesWithoutMesh);
	UE_ANALYSIS_LOG("Unsupported primitive component classes: %s", *MapToString(Results.UnsupportedPrimCounts));
	UE_ANALYSIS_LOG("---------------------------------------------------------------");
	UE_ANALYSIS_LOG("Physics Stats:");
	UE_ANALYSIS_LOG("Number of physics profiles: %i", Results.CollisionProfileCounts.Num());
	for (auto& Entry : Results.CollisionProfileCounts)
	{
		UE_ANALYSIS_LOG("- Profile '%s': %i", *Entry.Key.ToString(), Entry.Value);
	}
	UE_ANALYSIS_LOG("Custom collision meshes (%i, max 10):", Results.CustomCollisionMeshPaths.Num());
	for (auto& Entry : Results.CustomCollisionMeshPaths)
	{
		UE_ANALYSIS_LOG("- %s", *Entry);
	}
	UE_ANALYSIS_LOG("Total num static mesh components with physics:  %i", Results.NumStaticMeshComponentsWithPhysics);
	UE_ANALYSIS_LOG(
		"Avg num shapes per static mesh component: %i",
		(Results.NumStaticMeshComponentsWithPhysics > 0
			 ? Results.TotalShapeCount / Results.NumStaticMeshComponentsWithPhysics
			 : 0));
	UE_ANALYSIS_LOG("Shapes responses:");
	UE_ANALYSIS_LOG("- Query enabled: %i", Results.TotalNumPhysicsEnabled_Query);
	UE_ANALYSIS_LOG("- Physics enabled: %i", Results.TotalNumPhysicsEnabled_Physics);
	UE_ANALYSIS_LOG("- Probes enabled: %i", Results.TotalNumPhysicsEnabled_Probe);
	UE_ANALYSIS_LOG("Shapes by complexity:");
	UE_ANALYSIS_LOG("- total: %i (max %i per mesh)", Results.TotalShapeCount, Results.MaxShapeCount);
	UE_ANALYSIS_LOG("- cheap (box, sphere, capsule): %i", Results.MaxShapeCount - Results.MaxExpensiveShapeCount);
	UE_ANALYSIS_LOG(
		"- expensive (convex collision, other): %i (max %i per mesh)",
		Results.TotalExpensiveShapeCount,
		Results.MaxExpensiveShapeCount);
	UE_ANALYSIS_LOG("- biggest number of expensive collisions: %s", *Results.MaxExpensiveShapeOwner);
	UE_ANALYSIS_LOG("Shapes by AABB size:");
	UE_ANALYSIS_LOG("- small (<100uu): %i", Results.NumShapes_Small);
	UE_ANALYSIS_LOG("- medium (<500uu): %i", Results.NumShapes_Medium);
	UE_ANALYSIS_LOG("- large (>500uu): %i", Results.NumShapes_Large);
	UE_ANALYSIS_LOG("---------------------------------------------------------------");
#undef UE_ANALYSIS_LOG
#undef UE_ANALYSIS_LOG_CVAR

	UE_LOG(LogOpenUnrealUtilities, Log, TEXT(" \n%s"), *AnalysisLogString);
}

class FMaterialAnalysisOverlay : public OUU::Developer::FWorldStatsOverlay
{
public:
	FMaterialAnalysisOverlay() : FWorldStatsOverlay(::UpdateInterval)
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
		DrawCallsStats.DataSeries.Add({Buffer_MeshCombinations, FColorList::Yellow, "mat/collision combos"});

		auto& InstanceStats = GraphStats.AddDefaulted_GetRef();
		InstanceStats.Name = TEXT("mesh instances");
		InstanceStats.DataSeries.Add({Buffer_NumStaticMeshInstances_Max, FColorList::Violet, "max"});
		InstanceStats.DataSeries.Add({Buffer_NumStaticMeshInstances_Now, FColorList::Red, "now"});
		InstanceStats.DataSeries.Add({Buffer_NumStaticMeshInstances_Possible, FColorList::Green, "best"});

		auto& ShapeCounts = GraphStats.AddDefaulted_GetRef();
		ShapeCounts.Name = TEXT("physics (total)");
		ShapeCounts.DataSeries.Add({Buffer_SMCollision_TotalShapeCount, FColorList::Violet, "all shapes"});
		ShapeCounts.DataSeries.Add({Buffer_SMCollision_ExpensiveShapeCount, FColorList::Red, "expensive shapes"});
		ShapeCounts.DataSeries.Add({Buffer_SMCollision_NumCollisionsEnabled_Query, FColorList::Yellow, "query"});
		ShapeCounts.DataSeries.Add({Buffer_SMCollision_NumCollisionsEnabled_Physics, FColorList::Green, "physics"});
		ShapeCounts.DataSeries.Add({Buffer_SMCollision_NumCollisionsEnabled_Probe, FColorList::Blue, "probe"});

		auto& PerMeshShapes = GraphStats.AddDefaulted_GetRef();
		PerMeshShapes.Name = TEXT("physics (per mesh)");
		PerMeshShapes.DataSeries.Add({Buffer_SMCollision_AvgShapeCount, FColorList::Violet, "avg shapes"});
		PerMeshShapes.DataSeries.Add({Buffer_SMCollision_MaxShapeCount, FColorList::Green, "max shapes"});
		PerMeshShapes.DataSeries.Add(
			{Buffer_SMCollision_MaxExpensiveShapeCount, FColorList::Red, "max expensive shapes"});

		auto& ShapeSizes = GraphStats.AddDefaulted_GetRef();
		ShapeSizes.Name = TEXT("physics (shapes by size)");
		ShapeSizes.DataSeries.Add({Buffer_SMCollision_NumShapeSizes_Small, FColorList::Red, "small"});
		ShapeSizes.DataSeries.Add({Buffer_SMCollision_NumShapeSizes_Medium, FColorList::Green, "medium"});
		ShapeSizes.DataSeries.Add({Buffer_SMCollision_NumShapeSizes_Large, FColorList::Yellow, "large"});

		auto& PhysicsProfiles = GraphStats.AddDefaulted_GetRef();
		PhysicsProfiles.Name = TEXT("physics (profiles)");
		PhysicsProfiles.DataSeries.Add(
			{Buffer_SMCollision_NumCollisionProfiles, FColorList::LightBlue, "num profiles"});
	}

private:
	// Component stats
	TCircularAggregator<float> Buffer_ComponentsNow{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_ComponentsBest{NumFramesForBuffer};

	// Draw calls
	TCircularAggregator<float> Buffer_DrawCallsNow{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_DrawCallsBest{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_Materials{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_MeshCombinations{NumFramesForBuffer};

	// Instances
	TCircularAggregator<float> Buffer_NumStaticMeshInstances_Max{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_NumStaticMeshInstances_Now{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_NumStaticMeshInstances_Possible{NumFramesForBuffer};

	// Physics
	TCircularAggregator<float> Buffer_SMCollision_TotalShapeCount{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_SMCollision_ExpensiveShapeCount{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_SMCollision_AvgShapeCount{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_SMCollision_MaxShapeCount{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_SMCollision_MaxExpensiveShapeCount{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_SMCollision_NumCollisionProfiles{NumFramesForBuffer};
	// query|physics|probe
	TCircularAggregator<float> Buffer_SMCollision_NumCollisionsEnabled_Query{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_SMCollision_NumCollisionsEnabled_Physics{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_SMCollision_NumCollisionsEnabled_Probe{NumFramesForBuffer};

	// size tracking
	TCircularAggregator<float> Buffer_SMCollision_NumShapeSizes_Small{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_SMCollision_NumShapeSizes_Medium{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_SMCollision_NumShapeSizes_Large{NumFramesForBuffer};

	// not aggregated, just show the current frame:
	TMap<FName, int32> CollisionProfileCounts;
	FString MaxExpensiveShapeOwner;
	TArray<FString> CustomCollisionMeshPaths;

	// - TWorldStatsOverlay
	void TickStats(UWorld* TargetWorld) override
	{
		const bool bUseLogarithmicYAxis = CVarUseLogarithmicYAxis.GetValueOnAnyThread();
		for (auto& Entry : GraphStats)
		{
			Entry.bUseLogarithmicAxis = bUseLogarithmicYAxis;
		}

		auto Results = AnalyzeMaterialUsage(TargetWorld);

		// Components
		Buffer_ComponentsNow.Add(Results.MeshStatsSum.NumStaticMeshComponentsNow);
		Buffer_ComponentsBest.Add(Results.NumStaticMeshComponents_Best);

		// Draw calls
		Buffer_DrawCallsNow.Add(Results.DrawCalls_Current);
		Buffer_DrawCallsBest.Add(Results.DrawCalls_Best);
		Buffer_Materials.Add(Results.NumUniqueMaterials);
		Buffer_MeshCombinations.Add(Results.MeshStatsByCombo.Num());

		// Instances
		Buffer_NumStaticMeshInstances_Max.Add(Results.MeshStatsSum.NumStaticMeshInstances_Max);
		Buffer_NumStaticMeshInstances_Now.Add(Results.MeshStatsSum.NumStaticMeshInstances_Now);
		Buffer_NumStaticMeshInstances_Possible.Add(Results.MeshStatsSum.NumStaticMeshInstances_Possible);

		// Physics
		Buffer_SMCollision_TotalShapeCount.Add(Results.TotalShapeCount);
		Buffer_SMCollision_ExpensiveShapeCount.Add(Results.TotalExpensiveShapeCount);
		Buffer_SMCollision_MaxShapeCount.Add(Results.MaxShapeCount);
		Buffer_SMCollision_MaxExpensiveShapeCount.Add(Results.MaxExpensiveShapeCount);
		if (Results.NumStaticMeshComponentsWithPhysics > 0)
		{
			Buffer_SMCollision_AvgShapeCount.Add(Results.TotalShapeCount / Results.NumStaticMeshComponentsWithPhysics);
		}
		else
		{
			Buffer_SMCollision_AvgShapeCount.Add(0);
		}

		Buffer_SMCollision_NumCollisionProfiles.Add(Results.CollisionProfileCounts.Num());
		Buffer_SMCollision_NumCollisionsEnabled_Query.Add(Results.TotalNumPhysicsEnabled_Query);
		Buffer_SMCollision_NumCollisionsEnabled_Physics.Add(Results.TotalNumPhysicsEnabled_Physics);
		Buffer_SMCollision_NumCollisionsEnabled_Probe.Add(Results.TotalNumPhysicsEnabled_Probe);

		Buffer_SMCollision_NumShapeSizes_Small.Add(Results.NumShapes_Small);
		Buffer_SMCollision_NumShapeSizes_Medium.Add(Results.NumShapes_Medium);
		Buffer_SMCollision_NumShapeSizes_Large.Add(Results.NumShapes_Large);

		CollisionProfileCounts = Results.CollisionProfileCounts;
		MaxExpensiveShapeOwner = MoveTemp(Results.MaxExpensiveShapeOwner);
		CustomCollisionMeshPaths = MoveTemp(Results.CustomCollisionMeshPaths);
	}

	void OnDrawDebug(UCanvas* InCanvas) const override
	{
		FWorldStatsOverlay::OnDrawDebug(InCanvas);

		InCanvas->Canvas->DrawShadowedString(
			50.f,
			50.f,
			*FString::Printf(
				TEXT("Most Expensive Physics Shape Owner (%i shapes): %s"),
				Buffer_SMCollision_MaxExpensiveShapeCount.HasData()
					? static_cast<int32>(Buffer_SMCollision_MaxExpensiveShapeCount.Last())
					: 0,
				*MaxExpensiveShapeOwner),
			GEngine->GetSmallFont(),
			FColor::White);

		InCanvas->Canvas->DrawShadowedString(
			50.f,
			65.f,
			*FString::Printf(TEXT("Collision Profiles: %s"), *MapToString(CollisionProfileCounts)),
			GEngine->GetSmallFont(),
			FColor::White);

		InCanvas->Canvas->DrawShadowedString(
			50.f,
			80.f,
			*FString::Printf(
				TEXT("Custom Collision Components (max first 10):\n%s"),
				*ArrayToString(CustomCollisionMeshPaths, TEXT("\n"))),
			GEngine->GetSmallFont(),
			FColor::White);
	}
};

DEFINE_OUU_WORLD_STAT_OVERLAY(
	FMaterialAnalysisOverlay,
	STATIC_MESH_ANALYSIS_BASE_CVAR,
	"Toggle displaying stats of static meshes and their materials in the current world as on-screen graphs")

static FAutoConsoleCommand AnalyzeMaterialUsage_Command(
	TEXT(STATIC_MESH_ANALYSIS_BASE_CVAR ".Dump"),
	TEXT("Write stats of static meshes and their materials in the current world to the log"),
	FConsoleCommandDelegate::CreateStatic([]() { DumpMaterialAnalysis(FMaterialAnalysisOverlay::GetStatsWorld()); }));
