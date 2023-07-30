// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

// Macro implementation details for LiteralGameplayTag.h
// Not meant to be included directly!

/**
 * Call like this: RESOLVE_OPTIONAL_FLAGS(,##__VA_ARGS__, __VA_ARGS__, Fallback))
 *
 * Works like this: This macro always picks the third element, so...
 * ...either resolves to (if no variadic args are passed)
 *     RESOLVE_OPTIONAL_FLAGS( , ,Fallback) -> Fallback
 *                            1 2 3
 * ...or to (if a single variadic arg is passed)
 *     RESOLVE_OPTIONAL_FLAGS( ,Flag,Flag,Fallback) -> Flag
 *                            1 2    3
 * First dummy arg is required, otherwise '##__VA_ARGS__' doesn't work properly
 * If more than one va_arg param is passed, it overflows to ...
 */
#define RESOLVE_OPTIONAL_FLAGS(dummy, buffer, Flags, ...) Flags

#define OUU_DECLARE_GAMEPLAY_TAGS_START_IMPL(MODULE_API, TagType, RootTagName, Description, InFlags)                   \
	struct TagType;                                                                                                    \
	extern TagType TagType##_Instance;                                                                                 \
	struct MODULE_API TagType : public TLiteralGameplayTag<TagType, TagType, TagType>                                  \
	{                                                                                                                  \
	public:                                                                                                            \
		static const ELiteralGameplayTagFlags Flags = AutoConvertLiteralGameplayTagFlags(InFlags);                     \
		static const bool bAutoAddNativeTag = bool(Flags & ELiteralGameplayTagFlags::AutoRegister);                    \
		static const TagType& GetInstance() { return TagType##_Instance; }                                             \
		PRIVATE_OUU_GTAG_COMMON_FUNCS_IMPL(RootTagName, Description)                                                   \
		PRIVATE_OUU_GTAG_GETTER_IMPL(TagType, TagType)                                                                 \
	public:

// clang-format off
#define OUU_DECLARE_GAMEPLAY_TAGS_END_IMPL(TypeName)                                                                   \
	};
// clang-format on

#define OUU_GTAG_GROUP_START_IMPL(TagType, TagDescription, InFlags)                                                    \
	struct TagType : public TLiteralGameplayTag<TagType, SelfTagType, RootTagType>                                     \
	{                                                                                                                  \
		static const ELiteralGameplayTagFlags Flags = AutoConvertLiteralGameplayTagFlags(InFlags);                     \
		static const bool bAutoAddNativeTag = bool(Flags & ELiteralGameplayTagFlags::AutoRegister);                    \
		PRIVATE_OUU_GTAG_COMMON_FUNCS_IMPL(PREPROCESSOR_TO_STRING(TagType), TagDescription)                            \
		PRIVATE_OUU_GTAG_GETTER_IMPL(TagType, RootTagType)                                                             \
	public:                                                                                                            \
		static const TagType& GetInstance() { return ParentTagType::GetInstance().TagType##_Instance; }

// clang-format off
#define OUU_GTAG_GROUP_END_IMPL(TagType)                                                                                    \
	};                                                                                                                 \
private:                                                                                                               \
	TagType TagType##_Instance;                                                                                        \
public:
// clang-format on

//---------------------------------------------------------------------------------------------------------------------

#define PRIVATE_OUU_GTAG_GETTER_IMPL(TagType, RootTagType)                                                             \
private:                                                                                                               \
	template <typename, typename, typename>                                                                            \
	friend struct TLiteralGameplayTag;                                                                                 \
	template <typename, ELiteralGameplayTagFlags, bool>                                                                \
	friend struct OUU::Runtime::Private::TConditionalNativeTagGetter;                                                  \
	OUU::Runtime::Private::TConditionalNativeTagGetter<TagType, Flags, bAutoAddNativeTag> NativeTagGetter;

#define PRIVATE_OUU_GTAG_COMMON_FUNCS_IMPL(TagName, TagDescription)                                                    \
public:                                                                                                                \
	static FString GetRelativeName() { return FString(TagName); }                                                      \
	static FString GetDescription() { return FString(TEXT(TagDescription)); }                                          \
	static FName GetModuleName() { return UE_MODULE_NAME; }                                                            \
	static FName GetPluginName() { return UE_PLUGIN_NAME; }
