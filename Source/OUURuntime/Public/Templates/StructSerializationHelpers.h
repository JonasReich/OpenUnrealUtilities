// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

namespace OUU::Runtime
{
	template <typename StructType>
	auto DefaultStructSerialization(StructType& StructRef, FArchive& Ar)
	{
		if (StructRef.StaticStruct()->UseBinarySerialization(Ar))
		{
			StructRef.StaticStruct()->SerializeBin(Ar, &StructRef);
		}
		else
		{
			StructRef.StaticStruct()->SerializeTaggedProperties(
				Ar,
				reinterpret_cast<uint8*>(&StructRef),
				StructRef.StaticStruct(),
				nullptr);
		}
	}

	template <typename StructType>
	auto DefaultStructSerialization(StructType& StructRef, FStructuredArchive::FSlot& Slot)
	{
		if (StructRef.StaticStruct()->UseBinarySerialization(Slot.GetUnderlyingArchive()))
		{
			StructRef.StaticStruct()->SerializeBin(Slot.GetUnderlyingArchive(), &StructRef);
		}
		else
		{
			StructRef.StaticStruct()->SerializeTaggedProperties(
				Slot.GetUnderlyingArchive(),
				reinterpret_cast<uint8*>(&StructRef),
				StructRef.StaticStruct(),
				nullptr);
		}
	}
} // namespace OUU::Runtime
