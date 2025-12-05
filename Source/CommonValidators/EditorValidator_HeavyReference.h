#pragma once

// Unreal
#include "EditorValidatorBase.h"
#include "AssetRegistry/IAssetRegistry.h"

// Local
#include "CommonValidatorsDeveloperSettings.h"

// Gen
#include "EditorValidator_HeavyReference.generated.h"

/**
 *
 */
UCLASS()
class COMMONVALIDATORS_API UEditorValidator_HeavyReference : public UEditorValidatorBase
{
	GENERATED_BODY()
	
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;

private:
	bool IsAssetIncluded(const UCommonValidatorsDeveloperSettings* const DevSettings, const UObject* const InAsset, const FAssetData& ThisAssetData);
	bool GetAssetData(const IAssetRegistry* const AssetRegistry, const FAssetIdentifier& FoundAssetId, FAssetData& OutAssetData);
	FAssetManagerDependencyQuery SetupDependencyQuery(const FName& AssetName);
	
};