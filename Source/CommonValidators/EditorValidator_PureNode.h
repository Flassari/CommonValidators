#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorBase.h"
#include "EditorValidator_PureNode.generated.h"

/**
 * 
 */
UCLASS()
class COMMONVALIDATORS_API UEditorValidator_PureNode : public UEditorValidatorBase
{
	GENERATED_BODY()

	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;

	bool IsMultiPinPureNode(class UK2Node* PureNode);

	// Indicates if the given pure node is whitelisted and should not be flagged as a multi-pin pure node.
	bool IsWhitelistedPureNode(class UK2Node* PureNode);

};