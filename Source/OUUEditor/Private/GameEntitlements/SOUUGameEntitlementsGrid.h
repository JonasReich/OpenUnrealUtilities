// Copyright (c) 2026 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "GameEntitlements/OUUGameEntitlementsResolver.h"
#include "GameplayTagContainer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class IPropertyHandle;
class SHeaderRow;
class UOUUGameEntitlementSettings;
struct FPropertyChangedChainEvent;

namespace OUU::Editor::Private::GameEntitlements
{
	enum class EGridColumnKind : uint8
	{
		Version,
		Collection,
		SteamDlc,
	};

	struct FGridColumn
	{
		FName Id;
		EGridColumnKind Kind = EGridColumnKind::Version;
		// Version or collection tag. Invalid for DLC columns.
		FGameplayTag Tag;
		int32 SteamDlcAppId = 0;
		FText Label;
		FText ToolTip;

		FGameplayTagContainer ExplicitTags;
		FGameplayTagContainer EffectiveTags;
		OUU::Runtime::GameEntitlements::FEntitlementSourceMap Sources;

		// Collection columns only: the collection (indirectly) contains itself.
		bool bHasCycle = false;
	};

	struct FGridRow
	{
		FGameplayTag Tag;
		bool bIsCollection = false;
		// Section header rows have no tag and only show Label in the name column.
		bool bIsSectionHeader = false;
		FText Label;
		FText ToolTip;
	};

	// Grid with one row per entitlement tag and one column per version, collection and Steam DLC.
	// Checkboxes edit the explicit assignments in UOUUGameEntitlementSettings::Entitlements, implicit inclusions are
	// marked inline.
	class SGameEntitlementsGrid : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SGameEntitlementsGrid)
			{
			}
		SLATE_END_ARGS()

		~SGameEntitlementsGrid() override;

		void Construct(
			const FArguments& InArgs,
			UOUUGameEntitlementSettings* InSettings,
			const TSharedRef<IPropertyHandle>& InEntitlementsHandle);

		TSharedRef<SWidget> MakeCellWidget(const TSharedPtr<FGridRow>& Row, const FName& ColumnId);

	private:
		void RefreshAll();
		void RebuildColumns();
		void RebuildRows();
		void RebuildHeader();
		void ApplyFilter();

		void RequestRefresh();
		EActiveTimerReturnType HandleRefreshTimer(double InCurrentTime, float InDeltaTime);

		void HandleSettingsChanged(FPropertyChangedChainEvent& PropertyChangedEvent);
		void HandleGameplayTagTreeChanged();
		void HandleSearchTextChanged(const FText& InText);

		TSharedRef<ITableRow> HandleGenerateRow(TSharedPtr<FGridRow> Row, const TSharedRef<STableViewBase>& OwnerTable);

		TSharedRef<SWidget> MakeNameCellWidget(const TSharedPtr<FGridRow>& Row) const;
		TSharedRef<SWidget> MakeHeaderWidget(const TSharedRef<FGridColumn>& Column) const;

		bool IsExplicit(const FGridRow& Row, const FGridColumn& Column) const;
		bool IsImplicit(const FGridRow& Row, const FGridColumn& Column) const;
		FText GetCellToolTip(const FGridRow& Row, const FGridColumn& Column) const;
		void ToggleExplicit(const FGridRow& Row, const FGridColumn& Column);

		TWeakObjectPtr<UOUUGameEntitlementSettings> WeakSettings;
		TSharedPtr<IPropertyHandle> EntitlementsHandle;

		TArray<TSharedRef<FGridColumn>> Columns;
		TMap<FName, TSharedRef<FGridColumn>> ColumnsById;

		TArray<TSharedPtr<FGridRow>> AllRows;
		TArray<TSharedPtr<FGridRow>> FilteredRows;
		FString FilterString;

		TSharedPtr<SHeaderRow> HeaderRow;
		TSharedPtr<SListView<TSharedPtr<FGridRow>>> ListView;

		FDelegateHandle SettingsChangedHandle;
		FDelegateHandle TagTreeChangedHandle;
		bool bIsRefreshPending = false;
	};
} // namespace OUU::Editor::Private::GameEntitlements
