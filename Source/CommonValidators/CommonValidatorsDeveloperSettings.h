#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "Animation/AnimBlueprint.h"
#include "Engine/DeveloperSettings.h"

#include "CommonValidatorsDeveloperSettings.generated.h"

USTRUCT(BlueprintType)
struct FCommonValidatorClassArray
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Common Validators")
	TArray<TSoftClassPtr<UObject>> ClassList;

	// Should this rule propagate to discovered children?
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Common Validators")
	bool AllowPropagationToChildren = true;

	bool MatchesClass(TSubclassOf<UObject> ClassToCheck) const
	{
		return ClassList.ContainsByPredicate([&](const TSoftClassPtr<UObject> &SoftClassPtr)
		   {
			   return AllowPropagationToChildren ? SoftClassPtr.IsValid() && ClassToCheck->IsChildOf(SoftClassPtr.Get()) : ClassToCheck.Get() == SoftClassPtr;
		   });	
	}
};

UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Common Validators"))
class COMMONVALIDATORS_API UCommonValidatorsDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/* Empty Tick Node Validator */
	// If true, we will validate for empty tick nodes.
	UPROPERTY(Config, EditAnywhere, Category="Common Validators|Empty Tick Node Validator")
	bool bEnableEmptyTickNodeValidator = true;
	
	//If true, we throw an error, otherwise a warning!
	UPROPERTY(Config, EditAnywhere, Category="Common Validators|Empty Tick Node Validator", meta = (EditCondition = "bEnableEmptyTickNodeValidator == true"))
	bool bErrorOnEmptyTickNodes = true;

	
	/* Pure Node Validator */
	// if true, we will validate for pure nodes being executed multiple times
	UPROPERTY(Config, EditAnywhere, Category="Common Validators|Pure Node Validator")
	bool bEnablePureNodeMultiExecValidator = true;
	
	//If true, we throw an error, otherwise a warning!
	UPROPERTY(Config, EditAnywhere, Category="Common Validators|Pure Node Validator", meta = (EditCondition = "bEnablePureNodeMultiExecValidator == true"))
	bool bErrorOnPureNodeMultiExec = true;

	// Methods/Nodes from classes in this list will be ignored by the pure node validator
	UPROPERTY(Config, EditAnywhere, Category="Common Validators|Pure Node Validator", meta = (EditCondition = "bEnablePureNodeMultiExecValidator == true"))
	TArray<FCommonValidatorClassArray> PureNodeValidatorHarmlessClasses = {
		{
			// FCommonValidatorClassArray
			{
				TSoftClassPtr<>(FSoftClassPath("/Script/Engine.KismetMathLibrary")),
				TSoftClassPtr<>(FSoftClassPath("/Script/Engine.KismetSystemLibrary")),
				TSoftClassPtr<>(FSoftClassPath("/Script/Engine.KismetTextLibrary")),
				TSoftClassPtr<>(FSoftClassPath("/Script/Engine.KismetRenderingLibrary")),
				TSoftClassPtr<>(FSoftClassPath("/Script/Engine.KismetMaterialLibrary")),
				TSoftClassPtr<>(FSoftClassPath("/Script/Engine.KismetArrayLibrary")),
				TSoftClassPtr<>(FSoftClassPath("/Script/Engine.GameplayStatics")),
				TSoftClassPtr<>(FSoftClassPath("/Script/Engine.KismetStringTableLibrary")),
				TSoftClassPtr<>(FSoftClassPath("/Script/Engine.KismetInternationalizationLibrary")),
				TSoftClassPtr<>(FSoftClassPath("/Script/Engine.KismetInputLibrary")),
				TSoftClassPtr<>(FSoftClassPath("/Script/Engine.KismetGuidLibrary")),
				TSoftClassPtr<>(FSoftClassPath("/Script/Engine.DataTableFunctionLibrary")),
				TSoftClassPtr<>(FSoftClassPath("/Script/Engine.BlueprintSetLibrary")),
				TSoftClassPtr<>(FSoftClassPath("/Script/Engine.BlueprintPlatformLibrary")),
				TSoftClassPtr<>(FSoftClassPath("/Script/Engine.BlueprintPathsLibrary")),
				TSoftClassPtr<>(FSoftClassPath("/Script/Engine.BlueprintMapLibrary")),
				TSoftClassPtr<>(FSoftClassPath("/Script/Engine.BlueprintInstancedStructLibrary")),
				TSoftClassPtr<>(FSoftClassPath("/Script/Engine.KismetNodeHelperLibrary"))
			},
			true
		}
	};

	
	/* Blocking Load Validator */
	// If true, we will validate for blocking loads in blueprints
	UPROPERTY(Config, EditAnywhere, Category="Common Validators|Blocking Load Validator")
	bool bEnableBlockingLoadValidator = true;
	
	//If true, we throw an error, otherwise a warning!
	UPROPERTY(Config, EditAnywhere, Category="Common Validators|Blocking Load Validator", meta = (EditCondition = "bEnableBlockingLoadValidator == true"))
	bool bErrorBlockingLoad = true;

	//If true, we ignore nodes which are disabled for compilation
	UPROPERTY(Config, EditAnywhere, Category="Common Validators|Blocking Load Validator", DisplayName="Ignore disabled nodes", meta = (EditCondition = "bEnableBlockingLoadValidator == true"))
	bool bBlockingLoadIgnoreDisabledNodes = true;

	
	/* Heavy Reference Validator */
	// If true, we will validate for references above the set value in blueprints
	UPROPERTY(Config, EditAnywhere, Category="Common Validators|Heavy Reference Validator")
	bool bEnableHeavyReferenceValidator = true;

	// If the total size on disk is above this, we consider the asset heavy
	UPROPERTY(Config, EditAnywhere, Category="Common Validators|Heavy Reference Validator", meta = (EditCondition = "bEnableHeavyReferenceValidator == true"))
	int MaximumAllowedReferenceSizeKiloBytes = 20480;

	// Whether an inability to gather the size of a child asset is an warning
	// This will prevent further context messages in most cases
	UPROPERTY(Config, EditAnywhere, Category="Common Validators|Heavy Reference Validator", meta = (EditCondition = "bEnableHeavyReferenceValidator == true"))
	bool bWarnOnUnsizableChildren = false;

	//If true, we throw an error, otherwise a warning!
	UPROPERTY(Config, EditAnywhere, Category="Common Validators|Heavy Reference Validator", meta = (EditCondition = "bEnableHeavyReferenceValidator == true"))
	bool bErrorHeavyReference = false;

	// Classes in this list, and their children, are ignored by heavy reference validator
	UPROPERTY(Config, EditAnywhere, Category="Common Validators|Heavy Reference Validator", meta = (EditCondition = "bEnableHeavyReferenceValidator == true"))
	TArray<TSoftClassPtr<UObject>> HeavyValidatorClassAndChildIgnoreList = {UAnimBlueprint::StaticClass()};

	// Classes in this list, and only classes in this list, are ignored by heavy reference validator
	UPROPERTY(Config, EditAnywhere, Category="Common Validators|Heavy Reference Validator", meta = (EditCondition = "bEnableHeavyReferenceValidator == true"))
	TMap<TSoftClassPtr<UObject>, FCommonValidatorClassArray> HeavyValidatorClassSpecificClassIgnoreList;

	UPROPERTY(Config, EditAnywhere, Category="Common Validators|Material Texture Sampler Validator")
	bool bEnableMaterialTextureSamplerValidator = true;

};