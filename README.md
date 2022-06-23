
# Open Unreal Utilities

![OUU logo](./Resources/ouu_wide.png)

The Open Unreal Utilities plugin is a general purpose utility plugin for Unreal Engine.

## Contents

The plugin is divided into several modules for different application purposes:

| Module              | Description  |
|---------------------|--------------|
| OUUBlueprintRuntime | Blueprint exposed functionality that is already present in C++ UE4 off-the-shelf |
| OUUDeveloper        | Developer tools, esp. debug commands that are available in editor and development builds |
| OUUEditor           | Blueprint function libraries for editor utilities |
| OUURuntime          | The core of the plugin. These are runtime utilities that are also available in shipping builds. These utilities should be useful in all kinds of different contexts ranging from container templates, execution flow helpers, etc |
| OUUTests            | Automated tests that test the plugin functionality. Only contains private content. _You will never need to reference this from other modules!_ |
| OUUUMG              | UMG Widgets and UI implementation helpers. <br> Many of these tools were deprecated in favor of UE5's CommonUI plugin. |

For a more detailed overview of all utilities you should check out the plugin source code,
as most of the documentation is provided in the form of source code comments.
If you want to know how a type is meant to be used, it can be useful to check the automation tests in the ``OUUTests`` module.

## Versions

### Plugin Versions

Plugin versions are flagged with tags. Check the [GitHub Releases](https://github.com/JonasReich/OpenUnrealUtilities/releases) for a brief summary of changes between versions. [master](https://github.com/JonasReich/OpenUnrealUtilities/tree/master) always points to the latest release.

> Latest plugin release: [v1.0.0](https://github.com/JonasReich/OpenUnrealUtilities/tree/v1.0.0) on [master](https://github.com/JonasReich/OpenUnrealUtilities/tree/master)

If you want stable versions, you should stick to the labeled release commits on the master branch.

### Unreal Versions

Active development happens on branches named by supported engine version (pattern: ``ue<major>.<minor>``, e.g. ``ue5.0``).

Multiple version branches may receive updates at the same time, e.g. even after the UE 5.0 release, I keep adding features to ue4.27 that are infrequently cross-merged to the ue5.0. This is because I'm still actively using the plugin on projects that do not always upgrade to the latest UE version.

> **active development:**
> - [ue4.26](https://github.com/JonasReich/OpenUnrealUtilities/tree/ue4.26)
> - [ue4.27](https://github.com/JonasReich/OpenUnrealUtilities/tree/ue4.27)
> - [ue5.0](https://github.com/JonasReich/OpenUnrealUtilities/tree/ue5.0)
> 
> old versions (not supported anymore):
> - [ue4.25](https://github.com/JonasReich/OpenUnrealUtilities/tree/ue4.25)

Features should never be merged back from higher engine versions to lower versions to guarantee asset compatibility. I'm generally trying to adopt a fast-forward merge philosophy, but as soon as version branches diverge, I have to resort to regular merges.

### Deprecation

Any major changes to the API will be assisted via the use of deprecation macros so you have time to upgrade your code.
Deprecations are phased out alongside with new minor version upgrades of Unreal Engine.

For example, changes introduced for UE5 that deprecate old types were marked with ``UE_DEPRECATED(5.0, "...")``.
These warnings may be removed as soon as I upgrade the plugin to support UE5.1, so at that point any code referencing the
deprecated symbols or headers will no longer compile.  

### Increasing Plugin Version

Follow the following steps to promote changes from develop to master:

1. Increase version number to the next desired version in the following three files:
	- [README.md](./README.md) _(this file)_ - Plugin Version section
	- [OpenUnrealUtilities.uplugin](./OpenUnrealUtilities.uplugin) VersionName field
	- [.version](./.version)
2. Summarize the most important changes included with the new version in the history summary section above
3. Submit only these changes (no functional code changes included) with message "Increased version to major.minor.patch", e.g. "Increased version to 0.6.0"
4. Tag the submit with annotated tag that matches the version number: Tag and annotation message should match exactly, e.g. both could be "v0.6.0"
5. Fast-forward merge develop into master
6. Push develop, master _and(!)_ tags

## Automated Testing

A big majority of the utilities has unit tests that can be executed via the Session Frontend from within Unreal.
These tests are written with the help of testing utilities contained in the OpenUnrealUtilitiesTests module.
Said testing utilities are prepared for usage in other plugins or game code. The actual tests should serve as examples how
both the testing utilities as well as the targeted components are meant to be used.

### Getting Started (with Automated Testing)

If you want to use the testing utilities, it should be sufficient to

- include OUUTestUtilities.h and
- add the OUUTestUtilities module to the private dependencies of the module implementing the tests

### Overview

The automation testing utilities contain the following components:

- **FAutomationTestWorld** is a utility object that allows creation of a simple world and game framework objects for tests
- **AutomationTestParameterParser** is a utility object for complex automation tests that allows easy parsing of test parameters
which are supplied as a single FString per test case
- **CollectionTestFunctions** are test functions similar to the FAutomationTestBase member functions TestTrue(), TestEqual(), etc.
that allow comparing collections to expected result values while adding some additional checks and error messages to aid in debugging failed tests
- **OUUTestMacros** is a collection of macros that simplify the creation of tests
	- ``OUU_*`` macros help declaring test cases with significantly less redundancy/boilerplate compared to test declaration macros provided out-of-the-box
	- ``SPEC_TEST_*`` macros are wrappers for test functions to be used in [Automation Specs] without adding duplicate descriptions for every test (see below)
- **UOUUTestObject and UOUUTestWidget** are not-abstract classes derived directly from UObject and UUserWidget respectively available to use in tests

### Automation Specs

Unreal Engine v4.22 added a new test type called [Automation Spec](https://docs.unrealengine.com/en-US/Programming/Automation/AutomationSpec/index.html)
following the [Behavior Driven Design (BDD)](https://en.wikipedia.org/wiki/Behavior-driven_development) methodology.

I am convinced this new approach of writing tests is better than the previous one, so most of the plugin tests are written as specs adhering to
the engine source code convention of placing specs in ```*.spec.cpp``` files. In cases where special test objects are required that need reflection code generated by
UHT, I put those into a FooTests.h header file next to spec file.

The macros used to define the specs are very lightweight and add minimal boilerplate, so I did not see any reason to add any macros for creating spec classes.
There is a set of macros for calling test functions though that wrap the automation test member functions as well the collection test functions added by the plugin.
All of these macros are prefixed with ``SPEC_TEST_`` so they are easy to recognize and auto-complete. The macros do not introduce any hidden logic (early returns, etc),
but they auto-fill the test function description which is passed as "What" parameter. The aim when using these functions is to enforce that test descriptions should be
part of the expectations formulated using Describe and It, so the actual test calls should not need any additional descriptions.

Whenever you run into instances where the description of the test case is not sufficient to explain the call of test functions,
it's a good sign that you should either rephrase the description or break the test case down into separate smaller test cases
that are easy to describe.

## Coding Conventions

The plugin adheres to the [Open Unreal Conventions](https://jonasreich.github.io/OpenUnrealConventions/) which are extended
coding conventions based on Epic Games' coding guidelines. 

Some plugin specific conventions:
- Types or functions that are not namespaced which are likely to cause naming conflicts with future engine types or third/project code should be prefixed with OUU
- The full name of the plugin 'OpenUnrealUtilities' should be avoided to keep type names short and consistent
- All module names in the plugin must start with OUU prefix to make them easily distinguishable in Dependencies lists of Build.cs files

## Licensing

Both Open Unreal Utilities plugin and [sample project](https://github.com/JonasReich/OpenUnrealSampleProject) are licensed under the MIT license.

See [LICENSE.md](LICENSE.md)

## Contributing

You are invited to create pull-requests to the github source for any additions or modifications you make to the plugin:
https://github.com/JonasReich/OpenUnrealUtilities
