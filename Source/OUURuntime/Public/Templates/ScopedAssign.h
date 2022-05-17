// Copyright (c) 2022 Jonas Reich

#pragma once

/**
 * Assign a value to a variable for the scope duration of this object.
 * The original value is restored after this object runs out of scope.
 */
template <class T>
struct TScopedAssign
{
private:
	T& Field;
	T OriginalValue;

public:
	TScopedAssign(T& InField, T NewValue) : Field(InField), OriginalValue(InField) { Field = NewValue; }
	~TScopedAssign() { Field = OriginalValue; }
};
