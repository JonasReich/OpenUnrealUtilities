// Copyright (c) 2020 Jonas Reich

#include "InterfaceUtilsTests.h"
#include "OUUTests.h"

#if WITH_AUTOMATION_WORKER

#include "Templates/InterfaceUtils.h"

#define OUU_TEST_CATEGORY OpenUnrealUtilities.InterfaceUtils

//////////////////////////////////////////////////////////////////////////
// C++ Interface
//////////////////////////////////////////////////////////////////////////

namespace InterfaceUtilsTests_CppInterface
{
	FORCEINLINE UObject* CreateTestObject()
	{
		return NewObject<UInterfaceUtilsTests_CppInterface_Impl>();
	}
}

#define OUU_TEST_TYPE CppInterface
#define TestInterface IInterfaceUtilsTests_CppInterface
#define TestNamespace InterfaceUtilsTests_CppInterface
#define ENABLE_BLUEPRINT_INTERFACE_TESTS 0
#include "Templates/InterfaceUtilsTests_IsValidInterface.inl"
#undef ENABLE_BLUEPRINT_INTERFACE_TESTS
#undef TestNamespace
#undef TestInterface

//////////////////////////////////////////////////////////////////////////

// Test if const script interface can be used to call non-const functions
OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(IsValid_ConstScriptInterface, DEFAULT_OUU_TEST_FLAGS)
{
	UInterfaceUtilsTests_CppInterface_Impl* Object = NewObject<UInterfaceUtilsTests_CppInterface_Impl>();
	const TScriptInterface<IInterfaceUtilsTests_CppInterface> ConstScriptInterface = Object;
	const TScriptInterface<IInterfaceUtilsTests_CppInterface>& ConstScriptInterfaceRef = ConstScriptInterface;

	ConstScriptInterfaceRef->SetNumber(42.f);

	TestEqual("Number set on const TScriptInterface<TestInterface>&", Object->GetNumber(), 42.f);

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(IsValidInterface_InterfacePtr, DEFAULT_OUU_TEST_FLAGS)
{
	UInterfaceUtilsTests_CppInterface_Impl* Object = NewObject<UInterfaceUtilsTests_CppInterface_Impl>();
	IInterfaceUtilsTests_CppInterface* InterfacePtr = Object;

	TestTrue("Object* is valid", IsValid(Object));
	TestTrue("Interface* is valid interface", IsValidInterface(InterfacePtr));

	Object = nullptr;
	InterfacePtr = nullptr;

	TestFalse("Object* is valid", IsValid(Object));
	TestFalse("Interface* is valid interface", IsValidInterface(InterfacePtr));

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(IsValid_ScriptInterface_LostValue, DEFAULT_OUU_TEST_FLAGS)
{
	UInterfaceUtilsTests_CppInterface_Impl* Object = NewObject<UInterfaceUtilsTests_CppInterface_Impl>();
	TScriptInterface<IInterfaceUtilsTests_CppInterface> ScriptInterface = Object;

	ScriptInterface.SetInterface(nullptr);
	if (IsValidInterface(ScriptInterface))
	{
		ScriptInterface->SetNumber(42.f);
	}

	TestEqual("Number set after validation", Object->GetNumber(), 42.f);

	return true;
}

//////////////////////////////////////////////////////////////////////////

#undef OUU_TEST_TYPE

//////////////////////////////////////////////////////////////////////////
// Blueprint Interface - C++ Implementation
//////////////////////////////////////////////////////////////////////////

namespace InterfaceUtilsTests_BpInterface_CppImpl
{
	FORCEINLINE UObject* CreateTestObject()
	{
		return NewObject<UInterfaceUtilsTests_BpInterface_CppImpl>();
	}
}

#define OUU_TEST_TYPE BpInterface_CppImpl
#define TestInterface IInterfaceUtilsTests_BpInterface
#define TestNamespace InterfaceUtilsTests_BpInterface_CppImpl
#define ENABLE_BLUEPRINT_INTERFACE_TESTS 1
#include "Templates/InterfaceUtilsTests_IsValidInterface.inl"
#undef ENABLE_BLUEPRINT_INTERFACE_TESTS
#undef TestNamespace
#undef TestInterface

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////

#undef OUU_TEST_TYPE

//////////////////////////////////////////////////////////////////////////
// Blueprint Interface - Blueprint Implementation
//////////////////////////////////////////////////////////////////////////

namespace InterfaceUtilsTests_BpInterface_BpImpl
{
	FORCEINLINE UObject* CreateTestObject()
	{
		UClass* BlueprintClass = StaticLoadClass(UObject::StaticClass(), NULL, TEXT("/OpenUnrealUtilities/Editor/Tests/BP_BpInterface_BpImpl.BP_BpInterface_BpImpl_C"), NULL, LOAD_None, NULL);
		return NewObject<UObject>(GetTransientPackage(), BlueprintClass);
	}
}

#define OUU_TEST_TYPE BpInterface_BpImpl
#define TestInterface IInterfaceUtilsTests_BpInterface
#define TestNamespace InterfaceUtilsTests_BpInterface_BpImpl
#define ENABLE_BLUEPRINT_INTERFACE_TESTS 1
#include "Templates/InterfaceUtilsTests_IsValidInterface.inl"
#undef ENABLE_BLUEPRINT_INTERFACE_TESTS
#undef TestNamespace
#undef TestInterface

//////////////////////////////////////////////////////////////////////////

#undef OUU_TEST_CATEGORY
#undef OUU_TEST_TYPE

#endif
