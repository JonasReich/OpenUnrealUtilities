// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Actor.h"
#include "Templates/BitmaskUtils.h"

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

#if WITH_EDITOR
	// Progress of the incremental world partition scan, so the overlay can outline the cells that are still pending.
	// Cells are indexed the same way as in SActorMap::UpdateQueryResults: X = Index / NumCellsByAxis.
	struct FWorldPartitionScanProgress
	{
		// Corner of the first cell in world space.
		FVector2D GridMin = FVector2D::ZeroVector;
		float CellSize = 0.f;
		uint32 NumCellsByAxis = 0;
		// Cells below this index have been processed by every query.
		uint32 NumProcessedCells = 0;

		uint32 GetNumCells() const { return NumCellsByAxis * NumCellsByAxis; }
		bool HasPendingCells() const { return NumProcessedCells < GetNumCells(); }
	};

	inline bool operator==(const FWorldPartitionScanProgress& Lhs, const FWorldPartitionScanProgress& Rhs)
	{
		return Lhs.GridMin == Rhs.GridMin && Lhs.CellSize == Rhs.CellSize && Lhs.NumCellsByAxis == Rhs.NumCellsByAxis
			&& Lhs.NumProcessedCells == Rhs.NumProcessedCells;
	}
#endif
} // namespace OUU::Developer::ActorMapWindow

DECLARE_BITMASK_OPERATORS(OUU::Developer::ActorMapWindow::EShowFlags)