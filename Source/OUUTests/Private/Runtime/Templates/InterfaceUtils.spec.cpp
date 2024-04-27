// Copyright (c) 2023 Jonas Reich & Contributors

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

	#include "Runtime/Templates/InterfaceUtilsTests_Implementations.h"
	#include "Templates/InterfaceUtils.h"
	#include "UObject/Package.h"

template <typename TestInterface, typename InterfaceUtilsSpecType>
class TInterfaceUtilsSpec
{
public:
	static void Define(InterfaceUtilsSpecType& Test)
	{
		Test.It("should return false if called on a valid object that doesn't implement the interface", [&]() {
			const UOUUTestObject* Object = NewObject<UOUUTestObject>();

			Test.SPEC_TEST_TRUE(IsValid(Object));
			Test.SPEC_TEST_FALSE(IsValidInterface<TestInterface>(Object));
		});

		Test.It("should return true if called on a valid object that does implement the interface", [&]() {
			Test.SPEC_TEST_TRUE(IsValid(Test.TargetObject));
			Test.SPEC_TEST_TRUE(IsValidInterface<TestInterface>(Test.TargetObject));
		});

		Test.It("should return false if called on an invalid object that used to implement the interface", [&]() {
			Test.TargetObject = nullptr;
			Test.SPEC_TEST_FALSE(IsValid(Test.TargetObject));
			Test.SPEC_TEST_FALSE(IsValidInterface<TestInterface>(Test.TargetObject));
		});

		Test.It("should return true if called on a valid TScriptInterface to the correct class", [&]() {
			TScriptInterface<TestInterface> ScriptInterface = Test.TargetObject;

			Test.SPEC_TEST_TRUE(IsValid(Test.TargetObject));
			Test.SPEC_TEST_TRUE(IsValidInterface(ScriptInterface));
		});

		Test.It("should return false if called on an invalid TScriptInterface to the correct class", [&]() {
			TScriptInterface<TestInterface> ScriptInterface = Test.TargetObject;
			ScriptInterface.SetObject(nullptr);
			Test.SPEC_TEST_FALSE(IsValidInterface(ScriptInterface));
		});

		Test.It("should return true when called on TScriptInterface with nullptr interface", [&]() {
			TScriptInterface<TestInterface> ScriptInterface = Test.TargetObject;
			ScriptInterface.SetInterface(nullptr);
			Test.SPEC_TEST_TRUE(IsValidInterface(ScriptInterface));
		});
	}
};

BEGIN_DEFINE_SPEC(FInterfaceUtilsSpec, "OpenUnrealUtilities.Runtime.Templates.InterfaceUtils", DEFAULT_OUU_TEST_FLAGS)
public:
	UObject* TargetObject;
	IInterfaceUtilsTests_CppInterface* InterfacePtr;

protected:
END_DEFINE_SPEC(FInterfaceUtilsSpec)
void FInterfaceUtilsSpec::Define()
{
	It("const ref to const script interface can be used to call non-const functions on target object", [this]() {
		TargetObject = NewObject<UInterfaceUtilsTests_CppInterface_Impl>();
		const TScriptInterface<IInterfaceUtilsTests_CppInterface> ConstScriptInterface = TargetObject;
		const TScriptInterface<IInterfaceUtilsTests_CppInterface>& ConstScriptInterfaceRef = ConstScriptInterface;

		ConstScriptInterfaceRef->SetNumber(42.f);

		SPEC_TEST_EQUAL(Cast<UInterfaceUtilsTests_CppInterface_Impl>(TargetObject)->GetNumber(), 42.f);
	});

	Describe("IsValidInterface", [this]() {
		Describe("when operating on interfaces that can only implemented in C++", [this]() {
			BeforeEach([this]() {
				const auto CppImplObject = NewObject<UInterfaceUtilsTests_CppInterface_Impl>();
				TargetObject = CppImplObject;
				InterfacePtr = CppImplObject;
			});

			TInterfaceUtilsSpec<IInterfaceUtilsTests_CppInterface, FInterfaceUtilsSpec>::Define(*this);

			It("should return true if the underlying object pointer is valid", [this]() {
				SPEC_TEST_TRUE(IsValid(TargetObject));
				SPEC_TEST_TRUE(IsValidInterface(InterfacePtr));
			});

			It("should return false if the underlying object pointer is invalid", [this]() {
				InterfacePtr = nullptr;
				TestFalse("Interface* is valid interface", IsValidInterface(InterfacePtr));
			});

			It("should restore the interface pointer of a script interface smart pointer so it can be used for "
			   "interface calls",
			   [this]() {
				   TScriptInterface<IInterfaceUtilsTests_CppInterface> ScriptInterface = TargetObject;

				   ScriptInterface.SetInterface(nullptr);
				   if (IsValidInterface(ScriptInterface))
				   {
					   ScriptInterface->SetNumber(42.f);
				   }

				   SPEC_TEST_EQUAL(Cast<UInterfaceUtilsTests_CppInterface_Impl>(TargetObject)->GetNumber(), 42.f);
				   SPEC_TEST_EQUAL(ScriptInterface.GetObject(), TargetObject);
				   SPEC_TEST_EQUAL(
					   StaticCast<IInterfaceUtilsTests_CppInterface*>(ScriptInterface.GetInterface()),
					   InterfacePtr);
			   });
		});

		Describe("when operating on hybrid interfaces that are implemented in C++", [this]() {
			BeforeEach([this]() { TargetObject = NewObject<UInterfaceUtilsTests_BpInterface_CppImpl>(); });

			TInterfaceUtilsSpec<IInterfaceUtilsTests_BpInterface, FInterfaceUtilsSpec>::Define(*this);
		});

		Describe("when operating on hybrid interfaces that are implemented in Blueprint", [this]() {
			BeforeEach([this]() {
				const UClass* BlueprintClass = StaticLoadClass(
					UObject::StaticClass(),
					nullptr,
					TEXT("/OpenUnrealUtilities/Editor/Tests/BP_BpInterface_BpImpl.BP_BpInterface_BpImpl_C"),
					nullptr,
					LOAD_None,
					nullptr);
				TargetObject = NewObject<UObject>(GetTransientPackage(), BlueprintClass);
			});

			TInterfaceUtilsSpec<IInterfaceUtilsTests_BpInterface, FInterfaceUtilsSpec>::Define(*this);
		});
	});

	Describe("CALL_INTERFACE", [this]() {
		It("should call a C++ implementation of a hybrid interface", [this]() {
			TargetObject = NewObject<UInterfaceUtilsTests_BpInterface_CppImpl>();
			TScriptInterface<IInterfaceUtilsTests_BpInterface> ScriptInterface = TargetObject;

			ScriptInterface.SetInterface(nullptr);
			if (IsValidInterface(ScriptInterface))
			{
				CALL_INTERFACE(IInterfaceUtilsTests_BpInterface, SetNumber, ScriptInterface, 42.f);
			}

			const float Result = CALL_INTERFACE(IInterfaceUtilsTests_BpInterface, GetNumber, ScriptInterface);
			SPEC_TEST_EQUAL(Result, 42.f);
		});

		It("should call a Blueprint implementation of a hybrid interface", [this]() {
			const UClass* BlueprintClass = StaticLoadClass(
				UObject::StaticClass(),
				nullptr,
				TEXT("/OpenUnrealUtilities/Editor/Tests/BP_BpInterface_BpImpl.BP_BpInterface_BpImpl_C"),
				nullptr,
				LOAD_None,
				nullptr);
			TargetObject = NewObject<UObject>(GetTransientPackage(), BlueprintClass);
			TScriptInterface<IInterfaceUtilsTests_BpInterface> ScriptInterface = TargetObject;

			ScriptInterface.SetInterface(nullptr);
			if (IsValidInterface(ScriptInterface))
			{
				CALL_INTERFACE(IInterfaceUtilsTests_BpInterface, SetNumber, ScriptInterface, 42.f);
			}

			const float Result = CALL_INTERFACE(IInterfaceUtilsTests_BpInterface, GetNumber, ScriptInterface);
			SPEC_TEST_EQUAL(Result, 42.f);
		});
	});
}

#endif
