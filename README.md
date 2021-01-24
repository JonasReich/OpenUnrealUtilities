
# Open Unreal Utilities
The Open Unreal Utilities plugin is a general purpose utility plugin for Unreal Engine 4.

## Contents
So far the plugin mainly contains runtime utilities and some automated testing helpers. These utilities include:

- **Camera:** Blueprint utility functions for scene projections and render target handling
- **Flow Control:** Generic lock and request objects
- **Math:** Some math utility functions and blueprint exposure of FMath functionality
- **Templates:**
	- Container utilities (e.g. reverse iterator)
	- UE4 style adaptation of std::tie
	- Utility functions for interface pointer / TScriptInterface handling
	- Bitmask manipulation functions for native C++ and Blueprint compatible bitmask definitions
- **Traits:** Additional type traits
- **UMG:** Base widgets and utilities for input handling, user focus management and input bindings in widget contexts
- **Automated Testing:** *see [Automated Testing] section below*

For a more detailed overview of all utilities you should check out the plugin source code,
as most of the documentation is provided in the form of source code comments and automation tests.

## Versions
- Plugin version: 0.1.0
- Supported UE4 versions: 4.25

The plugin is still in a pre-1.0 development phase, so the API of many utilities is still up to change a lot.

Planned workflow after 1.0:

- Continue development on master
- Tag releases with plugin version number
- Add branches for supported engine versions (e.g. UE4.25), which are updated along with releases
- No updates of deprecated code. As soon as the plugin breaks for an old engine version we drop support for it
- UE4 Marketplace release is *not* planned at the moment, but possible

## Automated Testing
A big majority of the utilities has unit tests that can be executed via the Session Frontend from within Unreal.
These tests are written with the help of testing utilities contained in the OpenUnrealUtilitiesTests module.
Said testing utilities are prepared for usage in other plugins or game code. The actual tests should serve as examples how
both the testing utilities as well as the targeted components are meant to be used.

### Getting Started (with Automated Testing)
If you want to use the testing utilities, it should be sufficient to

- include OUUTests.h and
- add the OpenUnrealUtilitiesTests module to the private dependencies of the module implementing the tests

### Overview
The automation testing utilities contain the following components:

- **FAutomationTestWorld** is a utility object that allows creation of a simple world and game framework objects for tests
- **AutomationTestParameterParser** is a utility object for complex automation tests that allows easy parsing of test parameters
which are supplied as a single FString per test case
- **CollectionTestFunctions** are test functions similar to the FAutomationTestBase member functions TestTrue(), TestEqual(), etc.
that allow comparing collections to expected result values while adding some additional checks and error messages to aid in debugging failed tests
- **OUUTestMacros** is a collection of macros that simplify the creation of tests
	- OUU_\* macros help declaring test cases with significantly less redundancy/boilerplate compared to test declaration macros provided out-of-the-box
	- SPEC_TEST_\* macros are wrappers for test functions to be used in [Automation Specs] without adding duplicate descriptions for every test (see below)
- **UOUUTestObject and UOUUTestWidget** are not-abstract classes derived directly from UObject and UUserWidget respectively available to use in tests

### Automation Specs

Unreal Engine v4.22 added a new test type called [Automation Spec](https://docs.unrealengine.com/en-US/Programming/Automation/AutomationSpec/index.html)
following the [Behavior Driven Design (BDD)](https://en.wikipedia.org/wiki/Behavior-driven_development) methodology.

We are convinced this new approach of writing tests is better than the previous one, so most of the plugin tests are written as specs adhering to
the engine source code convention of placing specs in \*.spec.cpp files. In cases where special test objects are required that need reflection code generated by
UHT, we put those into a FooTests.h header file next to spec file.

The macros used to define the specs are very lightweight and add minimal boilerplate, so we did not see any reason to add any macros for creating spec classes.
There is a set of macros for calling test functions though that wrap the automation test member functions as well the collection test functions added by the plugin.
All of these macros are prefixed with SPEC_TEST_ so they are easy to recognize and auto-complete. The macros do not introduce any hidden logic (early returns, etc),
but they auto-fill the test function description which is passed as "What" parameter. The aim when using these functions is to enforce that test descriptions should be
part of the expectations formulated using Describe and It, so the actual test calls should not need any additional descriptions.

Whenever you run into instances where the description of the test case is not sufficient to explain the call of test functions, it's a good sign that you should either rephrase
the description or break the test case down into separate smaller test cases that are easy to describe.

## Licensing
Both Open Unreal Utilities plugin and [sample project](https://github.com/JonasReich/OpenUnrealUtilitiesSampleProject) are licensed under the MIT license.

## Contributing
You are invited to create pull-requests to the github source for any additions or modifications you make to the plugin:
https://github.com/JonasReich/OpenUnrealUtilities
