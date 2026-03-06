// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

namespace OUU::Runtime
{
	inline void DefaultStructSerialization(
		UScriptStruct& Struct,
		void* StructData,
		FArchive& Ar,
		const void* Defaults = nullptr)
	{
		if (Struct.UseBinarySerialization(Ar))
		{
			Struct.SerializeBin(Ar, StructData);
		}
		else
		{
			Struct.SerializeTaggedProperties(
				Ar,
				reinterpret_cast<uint8*>(StructData),
				&Struct,
				reinterpret_cast<const uint8*>(Defaults));
		}
	}

	inline void DefaultStructSerialization(
		UScriptStruct& Struct,
		void* StructData,
		FStructuredArchive::FSlot Slot,
		const void* Defaults = nullptr)
	{
		if (Struct.UseBinarySerialization(Slot.GetUnderlyingArchive()))
		{
			Struct.SerializeBin(Slot, StructData);
		}
		else
		{
			Struct.SerializeTaggedProperties(
				Slot,
				reinterpret_cast<uint8*>(StructData),
				&Struct,
				reinterpret_cast<const uint8*>(Defaults));
		}
	}

	template <typename StructType>
	auto DefaultStructSerialization(StructType& StructRef, FArchive& Ar, const void* Defaults = nullptr)
	{
		DefaultStructSerialization(*StructType::StaticStruct(), &StructRef, Ar, Defaults);
	}

	template <typename StructType>
	auto DefaultStructSerialization(
		StructType& StructRef,
		FStructuredArchive::FSlot Slot,
		const void* Defaults = nullptr)
	{
		DefaultStructSerialization(*StructType::StaticStruct(), &StructRef, Slot, Defaults);
	}
} // namespace OUU::Runtime
