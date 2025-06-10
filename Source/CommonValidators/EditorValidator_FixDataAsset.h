// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorBase.h"
#include "EditorValidator_FixDataAsset.generated.h"


UCLASS()
class COMMONVALIDATORS_API UFixDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fix")
	bool FixToTrue = false;
};

/**
 * 
 */
UCLASS()
class COMMONVALIDATORS_API UEditorValidator_FixDataAsset : public UEditorValidatorBase
{
	GENERATED_BODY()

public:

	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;

	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;

};
