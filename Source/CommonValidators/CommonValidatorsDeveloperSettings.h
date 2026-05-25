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


/** Configurable severity for settings-driven Blueprint validation rules. */
UENUM(BlueprintType)
enum class EEditorValidatorBlueprintValidationRuleSeverity : uint8
{
	Info,
	PerformanceWarning,
	Warning,
	Error
};

/** Controls whether a base Blueprint class is included when matching a validation rule. */
UENUM(BlueprintType)
enum class EEditorValidatorBlueprintBaseScope : uint8
{
	BaseAndDerived,
	DerivedOnly
};

/** Settings-driven rule for forbidding Blueprint implementations. */
USTRUCT(BlueprintType)
struct FEditorValidatorBlueprintImplementationRule
{
	GENERATED_BODY()

	/**
	 * Base class this rule applies to.
	 * Any Blueprint inheriting from this class may be validated.
	 */
	UPROPERTY(EditAnywhere, Config, Category="Implementation", meta=(ToolTip="The Blueprint base class this rule applies to."))
	TSoftClassPtr<UObject> BlueprintBaseClass;

	/**
	 * Determines whether validation applies to the selected class itself,
	 * or only Blueprints derived from it.
	 */
	UPROPERTY(EditAnywhere, Config, Category="Implementation", meta=(ToolTip="Whether the selected class itself or only derived classes are checked."))
	EEditorValidatorBlueprintBaseScope BaseScope = EEditorValidatorBlueprintBaseScope::DerivedOnly;

	/**
	 * Blueprint function or event that should not be overridden.
	 *
	 * Uses the Blueprint-visible name, not necessarily the native C++ name.
	 *
	 * Example:
	 *	Native: ActivateAbility()
	 *	Blueprint: K2_ActivateAbility
	 */
	UPROPERTY(EditAnywhere, Config, Category="Implementation", meta=(ToolTip="The Blueprint function or event to forbid. Use the Blueprint-visible name."))
	FName FunctionToForbid;

	/**
	 * Validation message shown when this rule is triggered.
	 *
	 * Example:
	 *	'Do not override ActivateAbility directly.'
	 */
	UPROPERTY(EditAnywhere, Config, Category="Message", meta=(MultiLine="true", ToolTip="Message shown when validation fails."))
	FText Message;

	/**
	 * Suggested fix shown alongside the validation message.
	 *
	 * Example:
	 *	'Move gameplay logic into OnActionStarted instead.'
	 */
	UPROPERTY(EditAnywhere, Config, Category="Message", meta=(MultiLine="true", ToolTip="Suggested fix or alternative workflow."))
	FString Suggestion;

	/**
	 * Determines how severe the validation issue appears in the editor.
	 *
	 * Warning:
	 *	General issue that should be fixed.
	 *
	 * Error:
	 *	Rule violation that should block usage.
	 *
	 * PerformanceWarning:
	 *	Issue related to runtime or scalability concerns.
	 *
	 * Info:
	 *	Non-blocking informational message.
	 */
	UPROPERTY(EditAnywhere, Config, Category="Message", meta=(ToolTip="Controls how severe this validation issue appears."))
	EEditorValidatorBlueprintValidationRuleSeverity Severity =
		EEditorValidatorBlueprintValidationRuleSeverity::Warning;
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

	UPROPERTY(EditAnywhere, Config, Category = "Blueprint Implementations")
	TArray<FEditorValidatorBlueprintImplementationRule> BlueprintImplementationRules;
};