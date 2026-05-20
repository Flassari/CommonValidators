// all rights reserved

#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorBase.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "UObject/Object.h"
#include "EditorValidator_MaterialTextureSampler.generated.h"

/**
 * 
 */
UCLASS()
class COMMONVALIDATORS_API UEditorValidator_MaterialTextureSampler : public UEditorValidatorBase
{
	GENERATED_BODY()

	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;

private:
	static TArray<const UMaterialExpressionTextureSample*> GetMaterialTextureSamplers(const UMaterial* Material);


};