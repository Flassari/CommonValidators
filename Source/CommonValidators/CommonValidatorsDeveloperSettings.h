#pragma once

#include "Engine/DeveloperSettings.h"

#include "CommonValidatorsDeveloperSettings.generated.h"

USTRUCT(BlueprintType)
struct FCommonValidatorClassArray
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<TSubclassOf<UObject>> ClassList;

	// Should this rule propagate to discovered children?
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool AllowPropagationToChildren = true;
};

UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Common Validators"))
class COMMONVALIDATORS_API UCommonValidatorsDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// If true, we will validate for empty tick nodes.
	UPROPERTY(Config, EditAnywhere)
	bool bEnableEmptyTickNodeValidator = true;
	
	//If true, we throw an error, otherwise a warning!
	UPROPERTY(Config, EditAnywhere, meta = (EditCondition = "bEnableEmptyTickNodeValidator == true"))
	bool bErrorOnEmptyTickNodes = true;
	
	// if true, we will validate for pure nodes being executed multiple times
	UPROPERTY(Config, EditAnywhere)
	bool bEnablePureNodeMultiExecValidator = true;
	
	//If true, we throw an error, otherwise a warning!
	UPROPERTY(Config, EditAnywhere, meta = (EditCondition = "bEnablePureNodeMultiExecValidator == true"))
	bool bErrorOnPureNodeMultiExec = true;
	
	// If true, we will validate for blocking loads in blueprints
	UPROPERTY(Config, EditAnywhere)
	bool bEnableBlockingLoadValidator = true;
	
	//If true, we throw an error, otherwise a warning!
	UPROPERTY(Config, EditAnywhere, meta = (EditCondition = "bEnableBlockingLoadValidator == true"))
	bool bErrorBlockingLoad = true;

	// If true, we will validate for references above the set value in blueprints
	UPROPERTY(Config, EditAnywhere)
	bool bEnableHeavyReferenceValidator = true;

	// If the total size on disk is above this, we consider the asset heavy
	UPROPERTY(Config, EditAnywhere, meta = (EditCondition = "bEnableHeavyReferenceValidator == true"))
	int MaximumAllowedReferenceSizeKiloBytes = 128;

	//If true, we throw an error, otherwise a warning!
	UPROPERTY(Config, EditAnywhere, meta = (EditCondition = "bEnableHeavyReferenceValidator == true"))
	bool bErrorHeavyReference = false;

	// Classes in this list, and their children, are ignored by heavy reference validator
	UPROPERTY(Config, EditAnywhere, meta = (EditCondition = "bEnableHeavyReferenceValidator == true"))
	TArray<TSubclassOf<UObject>> HeavyValidatorClassAndChildIgnoreList = {UAnimBlueprint::StaticClass()};

	// Classes in this list, and only classes in this list, are ignored by heavy reference validator
	UPROPERTY(Config, EditAnywhere, meta = (EditCondition = "bEnableHeavyReferenceValidator == true"))
	TMap<TSubclassOf<UObject>, FCommonValidatorClassArray> HeavyValidatorClassSpecificClassIgnoreList;
};