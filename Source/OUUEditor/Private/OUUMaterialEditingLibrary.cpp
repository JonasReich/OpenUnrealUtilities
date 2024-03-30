// Copyright (c) 2023 Jonas Reich & Contributors

#include "OUUMaterialEditingLibrary.h"

#include "IMaterialEditor.h"
#include "LogOpenUnrealUtilities.h"
#include "MaterialEditor/Public/MaterialEditingLibrary.h"
#include "MaterialEditorUtilities.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionMakeMaterialAttributes.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialFunctionInterface.h"
#include "ScopedTransaction.h"
#include "Toolkits/IToolkit.h"
#include "Toolkits/ToolkitManager.h"

void UOUUMaterialEditingLibrary::ConvertMaterialToMaterialAttributes(UMaterial* Material)
{
	if (!IsValid(Material))
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Failed to convert invalid material to material attributes"));
		return;
	}

	// Material already uses material attributes. No need to change anything
	if (Material->bUseMaterialAttributes)
		return;

	// Scoped transaction + modify for undo/redo and editor dirty flag
	const FScopedTransaction Transaction(INVTEXT("Convert to material attributes"));
	Material->Modify();

	const auto& MaterialAttributesInput = Material->GetExpressionInputForProperty(MP_MaterialAttributes);

	const FIntPoint NewNodeLocation{Material->EditorX, Material->EditorY};
	Material->EditorX += 300;

	UMaterialExpressionMakeMaterialAttributes* MakeMaterialAttributes =
		Cast<UMaterialExpressionMakeMaterialAttributes>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material,
			UMaterialExpressionMakeMaterialAttributes::StaticClass(),
			NewNodeLocation.X,
			NewNodeLocation.Y));

	CopyMaterialAttributeConnections(Material, MakeMaterialAttributes);

	// Switch the material to use material attributes
	Material->bUseMaterialAttributes = true;

	// Connect output 0 of new "make material attributes" node with material attributes results pin
	MaterialAttributesInput->Connect(0, MakeMaterialAttributes);
}

void UOUUMaterialEditingLibrary::InsertMaterialFunctionBeforeResult(
	UMaterial* Material,
	UMaterialFunction* MaterialFunction,
	bool bOnlyAddIfNotPresent /* = true */)
{
	if (!Material->bUseMaterialAttributes)
	{
		ConvertMaterialToMaterialAttributes(Material);
	}

	const auto& MaterialAttributesInput = Material->GetExpressionInputForProperty(MP_MaterialAttributes);
	if (const auto* PreviousLastNodeAsMaterialFunctionCall =
			Cast<UMaterialExpressionMaterialFunctionCall>(MaterialAttributesInput->Expression))
	{
		if (bOnlyAddIfNotPresent && PreviousLastNodeAsMaterialFunctionCall->MaterialFunction == MaterialFunction)
			return;
	}

	const FScopedTransaction Transaction(INVTEXT("Insert material function"));
	Material->Modify();

	const FIntPoint NewNodeLocation{Material->EditorX, Material->EditorY};
	Material->EditorX += 300;

	UMaterialExpressionMaterialFunctionCall* MaterialFunctionCall =
		Cast<UMaterialExpressionMaterialFunctionCall>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material,
			UMaterialExpressionMaterialFunctionCall::StaticClass(),
			NewNodeLocation.X,
			NewNodeLocation.Y));

	MaterialFunctionCall->SetMaterialFunction(MaterialFunction);

	if (auto* PreviousLastNode = MaterialAttributesInput->Expression)
	{
		// Connect the previous last node with our new function input
		const int32 PreviousOutputIndex = MaterialAttributesInput->OutputIndex;
		auto* pNewNodeInput = MaterialFunctionCall->GetInput(0);
		pNewNodeInput->Connect(PreviousOutputIndex, PreviousLastNode);
	}

	// Connect output 0 of new material function with material attributes
	MaterialAttributesInput->Connect(0, MaterialFunctionCall);
}

void UOUUMaterialEditingLibrary::OpenMaterialEditorAndJumpToExpression(UMaterialExpression* Expression)
{
	const auto* Outer = Expression->GetOuter();

	if (Outer->IsA<UMaterialInterface>())
	{
		FMaterialEditorUtilities::OnOpenMaterial(Outer);
	}
	else if (Outer->IsA<UMaterialFunctionInterface>())
	{
		FMaterialEditorUtilities::OnOpenFunction(Outer);
	}
	else
	{
		UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("Cannot open asset because outer is undefined"));
		return;
	}

	const TSharedPtr<IToolkit> FoundAssetEditor = FToolkitManager::Get().FindEditorForAsset(Outer);
	const TSharedPtr<IMaterialEditor> EditorPtr = StaticCastSharedPtr<IMaterialEditor>(FoundAssetEditor);

	if (EditorPtr.IsValid())
	{
		EditorPtr->FocusWindow();
		EditorPtr->JumpToExpression(Expression);
	}
}

void UOUUMaterialEditingLibrary::CopyInputConnection(const FExpressionInput* From, FExpressionInput* To)
{
	if (From && From->IsConnected())
	{
		To->Connect(From->OutputIndex, From->Expression);
	}
}

void UOUUMaterialEditingLibrary::CopyMaterialAttributeConnections(
	UMaterial* SourceMaterial,
	UMaterialExpressionMakeMaterialAttributes* TargetMakeMaterialAttributes)
{
	// For simple connections where the enum and input member share the exact same name.
#define COPY_INPUT_CONNECTION_SIMPLE(Property)                                                                         \
	CopyInputConnection(                                                                                               \
		SourceMaterial->GetExpressionInputForProperty(MP_##Property),                                                  \
		&TargetMakeMaterialAttributes->Property)

#define COPY_INPUT_CONNECTION_CUSTOM_UV(CustomUV)                                                                      \
	CopyInputConnection(                                                                                               \
		SourceMaterial->GetExpressionInputForProperty(MP_CustomizedUVs##CustomUV),                                     \
		&TargetMakeMaterialAttributes->CustomizedUVs[CustomUV])

	// clang-format off
	constexpr int32 LineBefore = __LINE__;
	COPY_INPUT_CONNECTION_SIMPLE(EmissiveColor);
	COPY_INPUT_CONNECTION_SIMPLE(Opacity);
	COPY_INPUT_CONNECTION_SIMPLE(OpacityMask);
	// MP_DiffuseColor -> used in Lightmass, not exposed to user, computed from: BaseColor, Metallic
	// MP_SpecularColor -> used in Lightmass, not exposed to user, derived from: SpecularColor, Metallic, Specular
	COPY_INPUT_CONNECTION_SIMPLE(BaseColor);
	COPY_INPUT_CONNECTION_SIMPLE(Metallic);
	COPY_INPUT_CONNECTION_SIMPLE(Specular);
	COPY_INPUT_CONNECTION_SIMPLE(Roughness);
	COPY_INPUT_CONNECTION_SIMPLE(Anisotropy);
	COPY_INPUT_CONNECTION_SIMPLE(Normal);
	COPY_INPUT_CONNECTION_SIMPLE(Tangent);
	COPY_INPUT_CONNECTION_SIMPLE(WorldPositionOffset);
	// MP_WorldDisplacement_DEPRECATED
	// MP_TessellationMultiplier_DEPRECATED
	COPY_INPUT_CONNECTION_SIMPLE(SubsurfaceColor);
	// MP_CustomData0 -> not exposed in material attributes
	// MP_CustomData1 -> not exposed in material attributes
	COPY_INPUT_CONNECTION_SIMPLE(AmbientOcclusion);
	COPY_INPUT_CONNECTION_SIMPLE(Refraction);
	COPY_INPUT_CONNECTION_CUSTOM_UV(0);
	COPY_INPUT_CONNECTION_CUSTOM_UV(1);
	COPY_INPUT_CONNECTION_CUSTOM_UV(2);
	COPY_INPUT_CONNECTION_CUSTOM_UV(3);
	COPY_INPUT_CONNECTION_CUSTOM_UV(4);
	COPY_INPUT_CONNECTION_CUSTOM_UV(5);
	COPY_INPUT_CONNECTION_CUSTOM_UV(6);
	COPY_INPUT_CONNECTION_CUSTOM_UV(7);
	COPY_INPUT_CONNECTION_SIMPLE(PixelDepthOffset);
	COPY_INPUT_CONNECTION_SIMPLE(ShadingModel);
	// MP_FrontMaterial -> not exposed in material attributes
	// MP_SurfaceThickness -> not exposed in material attributes
	constexpr int32 LineAfter = __LINE__;
	// clang-format on

#undef COPY_INPUT_CONNECTION_SIMPLE
#undef COPY_INPUT_CONNECTION_CUSTOM_UV

	static_assert(
		MP_EmissiveColor == 0 && MP_SurfaceThickness == 31 && MP_MaterialAttributes == 32,
		"The material property enum has changed, so this conversion probably misses some material property. Please "
		"check the engine source and fix this.");
	static_assert(
		LineBefore + static_cast<int32>(MP_MaterialAttributes) + 1 == LineAfter,
		"Number of CopyInputConnection() calls does not match the number of material properties.");
}
