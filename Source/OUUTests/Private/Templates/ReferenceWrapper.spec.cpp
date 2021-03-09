// Copyright (c) 2021 Jonas Reich

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

#include "Templates/ReferenceWrapper.h"

struct FRefWrapFoo
{
	int32 Number = 0;

	int32 operator&() const
	{
		return Number;
	}
};

BEGIN_DEFINE_SPEC(FReferenceWrapperSpec, "OpenUnrealUtilities.Templates.ReferenceWrapper", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FReferenceWrapperSpec)

void FReferenceWrapperSpec::Define()
{
	Describe("construction", [this]()
	{
		Describe("with explicit constructor call", [this]()
		{
			Describe("to primitive types", [this]()
			{
				It("should be possible from value object", [this]()
				{
					int32 i = 0;
					auto iRefWrapper = TReferenceWrapper<int32>(i);
					i = 42;
					SPEC_TEST_EQUAL(iRefWrapper, 42);
				});

				It("should be possible from reference", [this]()
				{
					int32 i = 0;
					int32& iRef = i;
					const auto iRefWrapper = TReferenceWrapper<int32>(iRef);
					i = 42;
					SPEC_TEST_EQUAL(iRefWrapper, 42);
				});

				It("should be possible from reference wrapper of matching type", [this]()
				{
					int32 i = 0;
					const auto iRefWrapperA = TReferenceWrapper<int32>(i);
					const auto iRefWrapperB = TReferenceWrapper<int32>(iRefWrapperA);
					i = 42;
					SPEC_TEST_EQUAL(iRefWrapperB, 42);
				});
			});

			Describe("to types with operator& overload", [this]()
			{
				It("should be possible from value object", [this]()
				{
					FRefWrapFoo Foo;
					const auto FooRefWrapper = TReferenceWrapper<FRefWrapFoo>(Foo);
					Foo.Number = 42;
					SPEC_TEST_EQUAL(FooRefWrapper.Get().Number, 42);
				});

				It("should be possible from reference", [this]()
				{
					FRefWrapFoo Foo;
					FRefWrapFoo& FooRef = Foo;
					const auto FooRefWrapper = TReferenceWrapper<FRefWrapFoo>(FooRef);
					Foo.Number = 42;
					SPEC_TEST_EQUAL(FooRefWrapper.Get().Number, 42);
				});

				It("should be possible from reference wrapper of matching type", [this]()
				{
					FRefWrapFoo Foo;
					const auto FooRefWrapperA = TReferenceWrapper<FRefWrapFoo>(Foo);
					const auto FooRefWrapperB = TReferenceWrapper<FRefWrapFoo>(FooRefWrapperA);
					Foo.Number = 42;
					SPEC_TEST_EQUAL(FooRefWrapperB.Get().Number, 42);
				});
			});
		});

		Describe("of non-const references with MakeRef()", [this]()
		{
			Describe("to primitive types", [this]()
			{
				It("should be possible from value object", [this]()
				{
					int32 i = 0;
					auto iRefWrapper = MakeRef<int32>(i);
					i = 42;
					SPEC_TEST_EQUAL(iRefWrapper, 42);
				});

				It("should be possible from reference", [this]()
				{
					int32 i = 0;
					int32& iRef = i;
					const auto iRefWrapper = MakeRef<int32>(iRef);
					i = 42;
					SPEC_TEST_EQUAL(iRefWrapper, 42);
				});

				It("should be possible from reference wrapper of matching type", [this]()
				{
					int32 i = 0;
					const auto iRefWrapperA = TReferenceWrapper<int32>(i);
					const auto iRefWrapperB = MakeRef<int32>(iRefWrapperA);
					i = 42;
					SPEC_TEST_EQUAL(iRefWrapperB, 42);
				});
			});

			Describe("to types with operator& overload", [this]()
			{
				It("should be possible from value object", [this]()
				{
					FRefWrapFoo Foo;
					const auto FooRefWrapper = MakeRef<FRefWrapFoo>(Foo);
					Foo.Number = 42;
					SPEC_TEST_EQUAL(FooRefWrapper.Get().Number, 42);
				});

				It("should be possible from reference", [this]()
				{
					FRefWrapFoo Foo;
					FRefWrapFoo& FooRef = Foo;
					const auto FooRefWrapper = MakeRef<FRefWrapFoo>(FooRef);
					Foo.Number = 42;
					SPEC_TEST_EQUAL(FooRefWrapper.Get().Number, 42);
				});

				It("should be possible from reference wrapper of matching type", [this]()
				{
					FRefWrapFoo Foo;
					const auto FooRefWrapperA = TReferenceWrapper<FRefWrapFoo>(Foo);
					const auto FooRefWrapperB = MakeRef<FRefWrapFoo>(FooRefWrapperA);
					Foo.Number = 42;
					SPEC_TEST_EQUAL(FooRefWrapperB.Get().Number, 42);
				});
			});
		});

		Describe("of const references with MakeConstRef()", [this]()
		{
			Describe("to primitive types", [this]()
			{
				It("should be possible from value object", [this]()
				{
					int32 i = 0;
					auto iRefWrapper = MakeConstRef<int32>(i);
					i = 42;
					SPEC_TEST_EQUAL(iRefWrapper, 42);
				});

				It("should be possible from reference", [this]()
				{
					int32 i = 0;
					int32& iRef = i;
					const auto iRefWrapper = MakeConstRef<int32>(iRef);
					i = 42;
					SPEC_TEST_EQUAL(iRefWrapper, 42);
				});

				It("should be possible from non-const reference wrapper of matching type", [this]()
				{
					int32 i = 0;
					const auto iRefWrapperA = TReferenceWrapper<int32>(i);
					const auto iRefWrapperB = MakeConstRef<int32>(iRefWrapperA);
					i = 42;
					SPEC_TEST_EQUAL(iRefWrapperB, 42);
				});

				It("should be possible from const reference wrapper of matching type", [this]()
				{
					int32 i = 0;
					const auto iRefWrapperA = TReferenceWrapper<const int32>(i);
					const auto iRefWrapperB = MakeConstRef<int32>(iRefWrapperA);
					i = 42;
					SPEC_TEST_EQUAL(iRefWrapperB, 42);
				});
			});

			Describe("to types with operator& overload", [this]()
			{
				It("should be possible from value object", [this]()
				{
					FRefWrapFoo Foo;
					const auto FooRefWrapper = MakeConstRef<FRefWrapFoo>(Foo);
					Foo.Number = 42;
					SPEC_TEST_EQUAL(FooRefWrapper.Get().Number, 42);
				});

				It("should be possible from reference", [this]()
				{
					FRefWrapFoo Foo;
					FRefWrapFoo& FooRef = Foo;
					const auto FooRefWrapper = MakeConstRef<FRefWrapFoo>(FooRef);
					Foo.Number = 42;
					SPEC_TEST_EQUAL(FooRefWrapper.Get().Number, 42);
				});

				It("should be possible from non-const reference wrapper of matching type", [this]()
				{
					FRefWrapFoo Foo;
					const auto FooRefWrapperA = TReferenceWrapper<FRefWrapFoo>(Foo);
					const auto FooRefWrapperB = MakeConstRef<FRefWrapFoo>(FooRefWrapperA);
					Foo.Number = 42;
					SPEC_TEST_EQUAL(FooRefWrapperB.Get().Number, 42);
				});

				It("should be possible from const reference wrapper of matching type", [this]()
				{
					FRefWrapFoo Foo;
					const auto FooRefWrapperA = TReferenceWrapper<const FRefWrapFoo>(Foo);
					const auto FooRefWrapperB = MakeConstRef<FRefWrapFoo>(FooRefWrapperA);
					Foo.Number = 42;
					SPEC_TEST_EQUAL(FooRefWrapperB.Get().Number, 42);
				});
			});
		});
	});

	Describe("assignment from reference wrappers", [this]()
	{
		It("should be allowed to regular non-const references with implicit conversion", [this]()
		{
			int32 i = 0;
			const TReferenceWrapper<int32> iRefWrapper = i;
			int32& iRef = iRefWrapper;
			iRef = 42;
			SPEC_TEST_EQUAL(i, 42);
		});

		It("should be allowed to values of the referenced type", [this]()
		{
			int32 i = 42;
			const TReferenceWrapper<int32> iRefWrapper = i;
			int32 iCopy = 0;
			iCopy = iRefWrapper;
			SPEC_TEST_EQUAL(iCopy, 42);
		});
	});

	It("should allow assigning new values to original object via Get()", [this]()
	{
		int32 i = 0;
		const TReferenceWrapper<int32> iRefWrapper = i;
		iRefWrapper.Get() = 42;
		SPEC_TEST_EQUAL(i, 42);
	});

	It("should allow assigning a different wrapper without changing the original object", [this]()
	{
		int32 i = 0;
		int32 j = 0;
		TReferenceWrapper<int32> iRefWrapper = i;
		iRefWrapper.Get() = 42;
		const TReferenceWrapper<int32> jRefWrapper = j;
		iRefWrapper = jRefWrapper;
		const int32 iRefWrapperTarget = iRefWrapper;
		SPEC_TEST_EQUAL(i, 42);
		SPEC_TEST_EQUAL(j, 0);
		SPEC_TEST_EQUAL(iRefWrapperTarget, 0);
	});

	Describe("storage in arrays", [this]()
	{
		It("should be allowed", [this]()
		{
			TArray<TReferenceWrapper<int32>> ReferenceWrappers;
			int32 i = 0;
			int32 j = 0;
			ReferenceWrappers.Add(i);
			ReferenceWrappers.Add(j);
			i = 42;
			j = 69;
			SPEC_TEST_EQUAL(ReferenceWrappers[0], 42);
			SPEC_TEST_EQUAL(ReferenceWrappers[1], 69);
		});

		It("should allow ranged based for loops", [this]()
		{
			TArray<TReferenceWrapper<int32>> ReferenceWrappers;
			int32 i = 0;
			int32 j = 0;
			ReferenceWrappers.Add(i);
			ReferenceWrappers.Add(j);
			i = 42;
			j = 69;
			TArray<int32> ResultArray;
			for (auto Element : ReferenceWrappers)
			{
				ResultArray.Add(Element);
			}
			SPEC_TEST_EQUAL(ResultArray[0], 42);
			SPEC_TEST_EQUAL(ResultArray[1], 69);
		});
	});

	It("should be allowed to be passed as parameter to functions that require a regular reference", [this]()
	{
		int32 i = 0;
		auto SomeFunction = [](int32& iRef) -> void { iRef = 42; };
		TReferenceWrapper<int32> IReferenceWrapper = i;
		SomeFunction(i);
		SPEC_TEST_EQUAL(i, 42);
	});
}

#endif
