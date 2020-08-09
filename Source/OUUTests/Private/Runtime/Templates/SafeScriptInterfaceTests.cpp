// Copyright (c) 2020 Jonas Reich

#include "SafeScriptInterfaceTests.h"
#include "OUUTests.h"

#if WITH_AUTOMATION_WORKER

#include "Templates/ArrayUtils.h"

#define OUU_TEST_CATEGORY OpenUnrealUtilities
#define OUU_TEST_TYPE SafeScriptInterface

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(AssignSafeScriptInterface, DEFAULT_OUU_TEST_FLAGS)
{
	USafeScriptInterfaceTestObjectTwo* Obj = NewObject<USafeScriptInterfaceTestObjectTwo>();
	USafeScriptInterfaceTestObject* Parent = NewObject<USafeScriptInterfaceTestObject>();

	Parent->TestInterface.SetObject(Obj);
	Parent->TestInterface.SetInterfaceFromObject();

	float f = Parent->TestInterface->ReturnFortyTwo();

	return true;
}

//////////////////////////////////////////////////////////////////////////

#undef OUU_TEST_CATEGORY
#undef OUU_TEST_TYPE

#endif
