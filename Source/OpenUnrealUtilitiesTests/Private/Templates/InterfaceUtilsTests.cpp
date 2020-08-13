// Copyright (c) 2020 Jonas Reich

#include "InterfaceUtilsTests.h"
#include "OUUTests.h"

#if WITH_AUTOMATION_WORKER

#include "Templates/InterfaceUtils.h"

#define OUU_TEST_CATEGORY OpenUnrealUtilities
#define OUU_TEST_TYPE InterfaceUtils

typedef IInterfaceUtilsTests_CppInterface TestInterface;

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(IsValidInterface_RawObject, DEFAULT_OUU_TEST_FLAGS)
{
	UOUUTestObject* Object = NewObject<UOUUTestObject>();

	TestTrue("Object* is valid", IsValid(Object));
	TestFalse("Object* is invalid interface", IsValidInterface<TestInterface>(Object));

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(IsValidInterface_ObjectPtr, DEFAULT_OUU_TEST_FLAGS)
{
	UInterfaceUtilsTests_CppInterface_Impl* Object = NewObject<UInterfaceUtilsTests_CppInterface_Impl>();

	TestTrue("Object* is valid", IsValid(Object));
	TestTrue("Object* is valid interface", IsValidInterface<TestInterface>(Object));

	Object = nullptr;

	TestFalse("Object* is valid", IsValid(Object));
	TestFalse("Object* is valid interface", IsValidInterface<TestInterface>(Object));

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(IsValidInterface_InterfacePtr, DEFAULT_OUU_TEST_FLAGS)
{
	UInterfaceUtilsTests_CppInterface_Impl* Object = NewObject<UInterfaceUtilsTests_CppInterface_Impl>();
	TestInterface* InterfacePtr = Object;
	
	TestTrue("Object* is valid", IsValid(Object));
	TestTrue("Interface* is valid interface", IsValidInterface(InterfacePtr));

	Object = nullptr;
	InterfacePtr = nullptr;
	
	TestFalse("Object* is valid", IsValid(Object));
	TestFalse("Interface* is valid interface", IsValidInterface(InterfacePtr));

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(IsValidInterface_ScriptInterface, DEFAULT_OUU_TEST_FLAGS)
{
	UInterfaceUtilsTests_CppInterface_Impl* Object = NewObject<UInterfaceUtilsTests_CppInterface_Impl>();
	TScriptInterface<TestInterface> ScriptInterface = Object;

	TestTrue("Object* is valid", IsValid(Object));
	TestTrue("TScriptInterface is valid interface", IsValidInterface(ScriptInterface));

	Object = nullptr;
	ScriptInterface.SetObject(nullptr);

	TestFalse("Object* is valid", IsValid(Object));
	TestFalse("Interface* is valid interface", IsValidInterface(ScriptInterface));

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(IsValid_ScriptInterface_LostValue, DEFAULT_OUU_TEST_FLAGS)
{
	UInterfaceUtilsTests_CppInterface_Impl* Object = NewObject<UInterfaceUtilsTests_CppInterface_Impl>();
	TScriptInterface<TestInterface> ScriptInterface = Object;

	ScriptInterface.SetInterface(nullptr);
	if (IsValidInterface(ScriptInterface))
	{
		ScriptInterface->SetNumber(42.f);
	}
	
	TestEqual("Number set after validation", Object->GetNumber(), 42.f);
	
	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(IsValid_ConstScriptInterface, DEFAULT_OUU_TEST_FLAGS)
{
	UInterfaceUtilsTests_CppInterface_Impl* Object = NewObject<UInterfaceUtilsTests_CppInterface_Impl>();
	const TScriptInterface<TestInterface> ConstScriptInterface = Object;
	const TScriptInterface<TestInterface>& ConstScriptInterfaceRef = ConstScriptInterface;

	ConstScriptInterfaceRef->SetNumber(42.f);
	
	TestEqual("Number set on const TScriptInterface<TestInterface>&", Object->GetNumber(), 42.f);

	return true;
}

//////////////////////////////////////////////////////////////////////////

#undef OUU_TEST_CATEGORY
#undef OUU_TEST_TYPE

#endif
