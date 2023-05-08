﻿// Copyright (c) 2023 Jonas Reich & Contributors

#ifndef OUU_DECLARE_CLASS_FLAGS
COMPILE_ERROR(
	"OUU_DECLARE_CLASS_FLAGS was not defined. Please do not include this file outside of OUUBlueprintClassFlags.cpp");
#endif

// clang-format off
OUU_DECLARE_CLASS_FLAGS(None),
OUU_DECLARE_CLASS_FLAGS(Abstract),
OUU_DECLARE_CLASS_FLAGS(DefaultConfig),
OUU_DECLARE_CLASS_FLAGS(Config),
OUU_DECLARE_CLASS_FLAGS(Transient),
OUU_DECLARE_CLASS_FLAGS(Optional),
OUU_DECLARE_CLASS_FLAGS(MatchedSerializers),
OUU_DECLARE_CLASS_FLAGS(ProjectUserConfig),
OUU_DECLARE_CLASS_FLAGS(Native),
OUU_DECLARE_CLASS_FLAGS(NotPlaceable),
OUU_DECLARE_CLASS_FLAGS(PerObjectConfig),
OUU_DECLARE_CLASS_FLAGS(ReplicationDataIsSetUp),
OUU_DECLARE_CLASS_FLAGS(EditInlineNew),
OUU_DECLARE_CLASS_FLAGS(CollapseCategories),
OUU_DECLARE_CLASS_FLAGS(Interface),
OUU_DECLARE_CLASS_FLAGS(Const),
OUU_DECLARE_CLASS_FLAGS(NeedsDeferredDependencyLoading),
OUU_DECLARE_CLASS_FLAGS(CompiledFromBlueprint),
OUU_DECLARE_CLASS_FLAGS(MinimalAPI),
OUU_DECLARE_CLASS_FLAGS(RequiredAPI),
OUU_DECLARE_CLASS_FLAGS(DefaultToInstanced),
OUU_DECLARE_CLASS_FLAGS(TokenStreamAssembled),
OUU_DECLARE_CLASS_FLAGS(HasInstancedReference),
OUU_DECLARE_CLASS_FLAGS(Hidden),
OUU_DECLARE_CLASS_FLAGS(Deprecated),
OUU_DECLARE_CLASS_FLAGS(HideDropDown),
OUU_DECLARE_CLASS_FLAGS(GlobalUserConfig),
OUU_DECLARE_CLASS_FLAGS(Intrinsic),
OUU_DECLARE_CLASS_FLAGS(Constructed),
OUU_DECLARE_CLASS_FLAGS(ConfigDoNotCheckDefaults),
OUU_DECLARE_CLASS_FLAGS(NewerVersionExists),
OUU_DECLARE_CLASS_FLAGS(Inherit),
OUU_DECLARE_CLASS_FLAGS(RecompilerClear),
OUU_DECLARE_CLASS_FLAGS(ShouldNeverBeLoaded),
OUU_DECLARE_CLASS_FLAGS(ScriptInherit),
OUU_DECLARE_CLASS_FLAGS(SaveInCompiledInClasses),
OUU_DECLARE_CLASS_FLAGS(AllFlags)
	// clang-format on