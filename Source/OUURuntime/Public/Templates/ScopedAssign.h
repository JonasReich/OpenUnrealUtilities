// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

/**
 * Assign a value to a variable for the scope duration of this object.
 * The original value is restored after this object runs out of scope.
 */
template <class T>
struct UE_DEPRECATED(5.1, "Use Unreal's built-in TGuardValue template instead of TScopedAssign") TScopedAssign
{
private:
	T& Field;
	T OriginalValue;

public:
	TScopedAssign(T& InField, T NewValue) : Field(InField), OriginalValue(InField) { Field = NewValue; }
	~TScopedAssign() { Field = OriginalValue; }
};
