// Copyright (c) 2023 Jonas Reich & Contributors

#include "Core/OUUDataTableLibrary.h"

bool UOUUDataTableLibrary::AddRowToDataTable(UDataTable* DataTable, FName RowName, FTableRowBase RowStruct)
{
	// We must never hit this! The real implementation is in Generic_AddRowToDataTable
	check(false);
	return false;
}

bool UOUUDataTableLibrary::RemoveRowFromDataTable(UDataTable* DataTable, FName RowName)
{
	if (!DataTable->GetRowNames().Contains(RowName))
	{
		return false;
	}
	DataTable->RemoveRow(RowName);
	return true;
}

DEFINE_FUNCTION(UOUUDataTableLibrary::execAddRowToDataTable)
{
	// Steps into the stack, walking to the next properties in it
	P_GET_OBJECT(UDataTable, DataTable);
	P_GET_PROPERTY(FNameProperty, RowName);

	Stack.StepCompiledIn<FStructProperty>(nullptr);
	void* RowPtr = Stack.MostRecentPropertyAddress;

	// We need this to wrap up the stack
	P_FINISH;

	// Grab the last property found when we walked the stack
	// This does not contains the property value, only its type information
	FStructProperty* StructProp = CastField<FStructProperty>(Stack.MostRecentProperty);

	bool bSuccess = false;
	if (!DataTable)
	{
		const FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AccessViolation,
			INVTEXT("Failed to resolve the table input. Be sure the DataTable is valid."));
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
	}
	else if (StructProp && RowPtr)
	{
		const UScriptStruct* OutputType = StructProp->Struct;
		const UScriptStruct* TableType = DataTable->GetRowStruct();

		// ReSharper disable once CppTooWideScope
		const bool bCompatible = (OutputType == TableType)
			|| (OutputType->IsChildOf(TableType) && FStructUtils::TheSameLayout(OutputType, TableType));
		if (bCompatible)
		{
			P_NATIVE_BEGIN;
			bSuccess = Generic_AddRowToDataTable(DataTable, RowName, StructProp, RowPtr);
			P_NATIVE_END;
		}
		else
		{
			const FBlueprintExceptionInfo ExceptionInfo(
				EBlueprintExceptionType::AccessViolation,
				INVTEXT("Incompatible input parameter; the data table's type is not the same as the type to add."));
			FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		}
	}
	else
	{
		const FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AccessViolation,
			INVTEXT("Failed to resolve the input parameter for AddRowToDataTable."));
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
	}

	*static_cast<bool*>(RESULT_PARAM) = bSuccess;
}

bool UOUUDataTableLibrary::Generic_AddRowToDataTable(
	UDataTable* DataTable,
	FName RowName,
	FStructProperty* StructProp,
	void* RowPtr)
{
	if (!IsValid(DataTable) || !RowPtr || !StructProp)
		return false;

	if (DataTable->GetRowMap().Contains(RowName))
		return false;

	const UScriptStruct* StructType = DataTable->GetRowStruct();
	if (StructType == nullptr)
		return false;

	const FTableRowBase* RowAsTableRowBase = StaticCast<FTableRowBase*>(RowPtr);

	DataTable->AddRow(RowName, *RowAsTableRowBase);
	return true;
}
