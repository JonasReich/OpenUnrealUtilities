
# Open Unreal Utilities

![OUU logo](./Resources/ouu.png)

The Open Unreal Utilities plugin is a general purpose utility plugin for Unreal Engine.

## Contents

The plugin is divided into several modules for different application purposes:

| Module              | Description  |
|---------------------|--------------|
| OUUBlueprintRuntime | Blueprint exposed functionality that is already present in C++ UE4 off-the-shelf |
| OUUDeveloper        | Developer tools, esp. debug commands that are available in editor and development builds |
| OUUEditor           | Blueprint function libraries for editor utilities |
| OUURuntime          | The core of the plugin. These are runtime utilities that are also available in shipping builds. These utilities should be useful in all kinds of different contexts ranging from container templates, execution flow helpers, etc |
| OUUTests            | Automated tests that test test the plugin functionality. Only contains private content. _You will never need to reference this from other modules!_ |
| OUUUMG              | UMG Widgets and UI implementation helpers. <br> Many of these tools were deprecated in favor of UE5's CommonUI plugin. |

For a more detailed overview of all utilities you should check out the plugin source code,
as most of the documentation is provided in the form of source code comments.
If you want to know how a type is meant to be used, it can be useful to check the automation tests in the ``OUUTests`` module.

## Workflows

### Versions

- Latest plugin version: 1.0.0
- Supported Unreal Engine versions: 4.26, 4.27, 5.0

Active support can only be provided for the latest version, but we try to maintain compatibility to the last two minor engine versions.

#### Deprecation

Any major changes to the API will be assisted via the use of deprecation macros so you have time to upgrade your code.
Deprecations are phased out alongside with new minor version upgrades of Unreal Engine.

For example, changes introduced for UE5 that deprecate old types were marked with ``UE_DEPRECATED(5.0, "...")``.
These warnings may be removed as soon as we upgrade the plugin to support UE5.1, so at that point any code referencing the
deprecated symbols or headers will no longer compile.  

#### Versions History (Summary)

Short summary of changes between minor versions so you don't have to sift through the entire changelog:

- **1.0.0**
  - Upgrade to UE 5.0
    - Updated code for new engine API  
    - Deprecated all OUUUMG utilities besides the ``UOUUWindow`` user widget class.
      Please use UE5's CommonUI plugin instead for managing widget layer stacks and input routing.
      Check out Epic's _LYRA_ sample project for reference.
  - Runtime
    - Added a string parser for gameplay tag queries - not very optimized, don't use at runtime (yet)
    - Added a template for read-write-locked variables to simplify access management
    - Added text library for dynamic list concatenation of FTexts
    - Added macro that enables bitmask operator support
    - Added Json library that allows serializing object properties from json
  - Blueprint
    - Added generic platform process library (allows launching external applications from Blueprint code)
    - Added property path helpers to get and set arbitrary property values from strings
    - Exposed object, class and property flags to Blueprint
    - Added new flow control Blueprint macros (also available for AnimInstances now)
    - Added clipboard library to read and write OS clipboard
  - Debugging utilities
    - Canvas graph plotting (used e.g. in sequential frame scheduler gameplay debugger)
    - Added animation gameplay debugger
  - Editor utilities
    - Added some generic editor asset actions
    - Added ouu.CompileAllBlueprints console command (same syntax as CompileAllBlueprints commandlet)
  - Code style/quality
    - Introduced clang-format file and unified/fixed formatting accordingly
    - Renamed namespaces to comply with implicit UE5 naming conventions
  - Other
    - Added texel checker materials
    - Added material functions for sRGB/linear conversion of entire vectors
    - Various bug fixes
- **0.8.0**
    - Upgrade to UE 4.27
    - Added lexical operators for Blueprint
    - Simplified message log functions and added UE_LOG-like macros
    - Reworked bitmask utilty functions to work with both engine enums and custom enums that implement OUU bitmask traits
    - Debugging tools
        - Added gameplay debugger category type list which simplifies registering of debugger categories
        - Added enhanced ability system gameplay debugger category
        - Added actor map window
        - Moved all debugging tools into new module OUUDeveloper
- **0.7.0**
    - Added plugin logo (matching to Open Unreal Conventions)
    - Improved clang compatibility for PS4/Linux targets (still not guaranteed to run on those platforms)
- **0.6.0**
    - New Utilities: Spiral IDs, GarbageCollectionListener
    - Various smaller bug fixes, improvements and cleanups
- **0.5.1**
    - Stability Fixes
- **0.5.0**
    - New Utilities: Literal Gameplay Tags, WorldBoundSFSchedulerRegistry
    - Renamed/moved a bunch of utilities to reduce conflicts with other projects
- **0.4.0**
    - Various fixes and stability improvements
    - Unified plugin log category usages
- **0.3.0**
    - Upgrade to UE 4.26
    - New Utilities: Ring Aggregator, Sequential Frame Scheduler
    - Module Cleanup
- **0.2.0** Split repo into master and develop branches
- **0.1.0** Base set of utilities (All changes prior to 0.2.0 with no clear versioning strategy)

#### Increasing Version

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

### Branch Strategy

- (Semi) stable releases for latest supported engine version on ``master``
- Active development for latest supported engine version on ``develop``
- Separate branches for supported engine versions (e.g. ``ue.27`` and ``ue5.0`` as of writing this) 
- Tag release commits on master with plugin version number, e.g. ``v0.1.0``

### Future Workflow Changes

- Add branches or tags for supported engine versions (e.g. UE4.25), which are updated along with releases
- No updates of deprecated code. As soon as the plugin breaks for an old engine version we drop support for it

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
	- OUU_\* macros help declaring test cases with significantly less redundancy/boilerplate compared to test declaration macros provided out-of-the-box
	- SPEC_TEST_\* macros are wrappers for test functions to be used in [Automation Specs] without adding duplicate descriptions for every test (see below)
- **UOUUTestObject and UOUUTestWidget** are not-abstract classes derived directly from UObject and UUserWidget respectively available to use in tests

### Automation Specs

Unreal Engine v4.22 added a new test type called [Automation Spec](https://docs.unrealengine.com/en-US/Programming/Automation/AutomationSpec/index.html)
following the [Behavior Driven Design (BDD)](https://en.wikipedia.org/wiki/Behavior-driven_development) methodology.

We are convinced this new approach of writing tests is better than the previous one, so most of the plugin tests are written as specs adhering to
the engine source code convention of placing specs in ```*.spec.cpp``` files. In cases where special test objects are required that need reflection code generated by
UHT, we put those into a FooTests.h header file next to spec file.

The macros used to define the specs are very lightweight and add minimal boilerplate, so we did not see any reason to add any macros for creating spec classes.
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

Both Open Unreal Utilities plugin and [sample project](https://github.com/JonasReich/OpenUnrealUtilitiesSampleProject) are licensed under the MIT license.

## Contributing

You are invited to create pull-requests to the github source for any additions or modifications you make to the plugin:
https://github.com/JonasReich/OpenUnrealUtilities
