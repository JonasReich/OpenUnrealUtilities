
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
	UObject* Object = TestNamespace::CreateTestObject();

	TestTrue("Object* is valid", IsValid(Object));
	TestTrue("Object* is valid interface", IsValidInterface<TestInterface>(Object));

	Object = nullptr;

	TestFalse("Object* is valid", IsValid(Object));
	TestFalse("Object* is valid interface", IsValidInterface<TestInterface>(Object));

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(IsValidInterface_ScriptInterface, DEFAULT_OUU_TEST_FLAGS)
{
	UObject* Object = TestNamespace::CreateTestObject();
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

#if ENABLE_BLUEPRINT_INTERFACE_TESTS

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(IsValid_ScriptInterface_LostValue, DEFAULT_OUU_TEST_FLAGS)
{
	UObject* Object = TestNamespace::CreateTestObject();
	TScriptInterface<TestInterface> ScriptInterface = Object;

	ScriptInterface.SetInterface(nullptr);
	if (IsValidInterface(ScriptInterface))
	{
		CALL_INTERFACE(TestInterface, SetNumber, ScriptInterface, 42.f);
	}

	float Result = CALL_INTERFACE(TestInterface, GetNumber, ScriptInterface);
	TestEqual("Number set after validation", Result, 42.f);

	return true;
}

#endif
