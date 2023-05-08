// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Engine/DataTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "OUUDataTableLibrary.generated.h"

/**
 * Extensions to UDataTableFunctionLibrary, i.e. functions to manipulate data tables
 * from Blueprint that are otherwise available from code.
 */
UCLASS()
class UOUUDataTableLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 * Adds a row to a data table. Fails if the row name already exists in the table.
	 * @param	DataTable	The data table to add the row to
	 * @param	RowName		Name key of the new row. Must be unique inside the table.
	 * @param	RowStruct	Custom structure to use as values for the new table row
	 * @returns				if the row was successfully added to the data table
	 */
	UFUNCTION(
		BlueprintCallable,
		CustomThunk,
		meta = (CustomStructureParam = "RowStruct"),
		Category = "Open Unreal Utilities|DataTable")
	static bool AddRowToDataTable(UDataTable* DataTable, FName RowName, FTableRowBase RowStruct);

	/**
	 * Removes a row from a data table if it exists.
	 * @param	DataTable	The data table to remove the row from
	 * @param	RowName		Name key of the row that should be removed
	 * @returns				if the row was successfully removed from the data table
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|DataTable")
	static bool RemoveRowFromDataTable(UDataTable* DataTable, FName RowName);

private:
	DECLARE_FUNCTION(execAddRowToDataTable);

	static bool Generic_AddRowToDataTable(
		UDataTable* DataTable,
		FName RowName,
		FStructProperty* StructProp,
		void* RowPtr);
};
