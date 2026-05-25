#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorBase.h"
#include "K2Node_CallFunction.h"
#include "Logging/TokenizedMessage.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "Engine/DeveloperSettings.h"
#include "EditorValidator_BlueprintRestriction.generated.h"

#define UE_API COMMONVALIDATORS_API

/*
Usage
-----

1. Create a validator by deriving from UEditorValidator_BlueprintRestriction.

2. Override AppliesTo() to restrict which assets the validator should scan.

3. Override BuildNodeRules() to block Blueprint events, overrides, or custom node patterns.

4. Override BuildCallRules() to block calls to specific functions.

Example:

	virtual void BuildNodeRules(TArray<FEditorValidatorBlueprintNodeRule>& OutRules) const override
	{
		OutRules.Add(
			FORBID_OVERRIDE(TEXT("ReceiveTick"))
				.Message(TEXT("Blueprint Tick is not allowed."))
				.Suggest(TEXT("Use a timer or native C++ update path instead."))
				.Warning()
		);
		
		OutRules.Add(
			FORBID_OVERRIDE(TEXT("K2_ActivateAbility"))
				.When([](const UBlueprint* Blueprint)
				{
					return Blueprint
						&& Blueprint->GeneratedClass
						&& Blueprint->GeneratedClass->IsChildOf(
							UMyActionGameplayAbility::StaticClass());
				})
				.Message(TEXT("Do not override ActivateAbility directly in UMyActionGameplayAbility."))
				.Suggest(TEXT("Move gameplay logic into OnActionStarted instead."))
				.Error()
		);
	}

	virtual void BuildCallRules(TArray<FEditorValidatorBlueprintCallRule>& OutRules) const override
	{
		OutRules.Add(
			FORBID_CALL(UGameplayStatics::StaticClass(), TEXT("GetAllActorsOfClass"))
				.Message(TEXT("GetAllActorsOfClass is not allowed in this Blueprint."))
				.Suggest(TEXT("Use a cached registry or subsystem lookup instead."))
				.PerformanceWarning()
		);
		
			OutRules.Add(
				FORBID_CALL(UGameplayStatics::StaticClass(), TEXT("PlaySound2D"))
					.When([](const UBlueprint* Blueprint)
					{
						return Blueprint
							&& Blueprint->GeneratedClass
							&& !Blueprint->GeneratedClass->IsChildOf(UUserWidget::StaticClass());
					})
					.Message(TEXT("PlaySound2D is only allowed from UI widgets. Gameplay Blueprints must route audio through the project audio subsystem."))
					.Suggest(TEXT("Use UGameAudioSubsystem::PlaySfx or an approved audio event wrapper instead."))
					.Error()
	);
	}
*/



/** User-facing fix suggestion shown alongside a validation violation. */
USTRUCT()
struct UE_API FEditorValidatorBlueprintSuggestion
{
	GENERATED_BODY()

	UPROPERTY()
	FText DisplayText;

	TOptional<FString> CopyText;
	TFunction<void()> OnExecute;

	bool bAllowCopy = false;

	FEditorValidatorBlueprintSuggestion() = default;

	explicit FEditorValidatorBlueprintSuggestion(const FText& InText)
		: DisplayText(InText)
	{
	}

	static FEditorValidatorBlueprintSuggestion MakeCopy(const FString& Label, const FString& CopyValue)
	{
		FEditorValidatorBlueprintSuggestion Suggestion(FText::FromString(Label));
		Suggestion.bAllowCopy = true;
		Suggestion.CopyText = CopyValue;
		return Suggestion;
	}

	static FEditorValidatorBlueprintSuggestion MakeAction(const FString& Label, TFunction<void()> Action)
	{
		FEditorValidatorBlueprintSuggestion Suggestion(FText::FromString(Label));
		Suggestion.OnExecute = MoveTemp(Action);
		return Suggestion;
	}
};

/** Generic node-level Blueprint validation rule. */
USTRUCT()
struct UE_API FEditorValidatorBlueprintNodeRule
{
	GENERATED_BODY()

	TFunction<bool(UEdGraphNode*, const UBlueprint*)> Matches;
	FText RuleText;

	TFunction<void(UEdGraphNode*, TArray<FEditorValidatorBlueprintSuggestion>&)> BuildSuggestions;
	TFunction<EMessageSeverity::Type(const UBlueprint*)> GetSeverity;
};

/** Function-call-specific Blueprint validation rule. */
USTRUCT()
struct UE_API FEditorValidatorBlueprintCallRule
{
	GENERATED_BODY()

	TFunction<bool(const UBlueprint*)> Applies;

	UPROPERTY()
	TObjectPtr<UClass> OwnerRootClass = nullptr;

	FName ForbiddenFunction;
	FText RuleText;

	TFunction<void(FName, TArray<FEditorValidatorBlueprintSuggestion>&)> BuildSuggestions;
	TFunction<EMessageSeverity::Type(const UBlueprint*)> GetSeverity;
};

/** Fluent builder for node-level validation rules. */
struct UE_API FEditorValidatorBlueprintNodeRuleBuilder
{
	FEditorValidatorBlueprintNodeRule Rule;
	TArray<FEditorValidatorBlueprintSuggestion> Suggestions;

	FEditorValidatorBlueprintNodeRuleBuilder& Message(const FString& In)
	{
		Rule.RuleText = FText::FromString(In);
		return *this;
	}

	FEditorValidatorBlueprintNodeRuleBuilder& When(TFunction<bool(const UBlueprint*)> Condition)
	{
		const auto PreviousMatch = Rule.Matches;

		Rule.Matches = [PreviousMatch, Condition = MoveTemp(Condition)](UEdGraphNode* Node, const UBlueprint* Blueprint)
		{
			return Condition(Blueprint) && PreviousMatch && PreviousMatch(Node, Blueprint);
		};

		return *this;
	}

	FEditorValidatorBlueprintNodeRuleBuilder& Suggest(const FString& In)
	{
		Suggestions.Add(FEditorValidatorBlueprintSuggestion(FText::FromString(In)));
		return *this;
	}

	FEditorValidatorBlueprintNodeRuleBuilder& SuggestCopy(const FString& Label, const FString& CopyValue)
	{
		Suggestions.Add(FEditorValidatorBlueprintSuggestion::MakeCopy(Label, CopyValue));
		return *this;
	}

	FEditorValidatorBlueprintNodeRuleBuilder& SuggestAction(const FString& Label, TFunction<void()> Action)
	{
		Suggestions.Add(FEditorValidatorBlueprintSuggestion::MakeAction(Label, MoveTemp(Action)));
		return *this;
	}

	FEditorValidatorBlueprintNodeRuleBuilder& Info()
	{
		Rule.GetSeverity = [](const UBlueprint*) { return EMessageSeverity::Info; };
		return *this;
	}

	FEditorValidatorBlueprintNodeRuleBuilder& PerformanceWarning()
	{
		Rule.GetSeverity = [](const UBlueprint*)
			{
				return EMessageSeverity::Warning;
			};

		const FText ExistingMessage = Rule.RuleText;

		Rule.RuleText = FText::Format(
			NSLOCTEXT(
				"EditorValidator",
				"PerformanceWarning",
				"[Performance] {0}"),
			ExistingMessage
		);

		return *this;
	}

	FEditorValidatorBlueprintNodeRuleBuilder& Warning()
	{
		Rule.GetSeverity = [](const UBlueprint*) { return EMessageSeverity::Warning; };
		return *this;
	}

	FEditorValidatorBlueprintNodeRuleBuilder& Error()
	{
		Rule.GetSeverity = [](const UBlueprint*) { return EMessageSeverity::Error; };
		return *this;
	}

	FEditorValidatorBlueprintNodeRuleBuilder& CustomSeverity(TFunction<EMessageSeverity::Type(const UBlueprint*)> In)
	{
		Rule.GetSeverity = MoveTemp(In);
		return *this;
	}

	operator FEditorValidatorBlueprintNodeRule()
	{
		Rule.BuildSuggestions =
			[LocalSuggestions = Suggestions](UEdGraphNode*, TArray<FEditorValidatorBlueprintSuggestion>& Out)
			{
				Out = LocalSuggestions;
			};

		return Rule;
	}
};

/** Fluent builder for function-call validation rules. */
struct UE_API FEditorValidatorBlueprintCallRuleBuilder
{
	FEditorValidatorBlueprintCallRule Rule;
	TArray<FEditorValidatorBlueprintSuggestion> Suggestions;

	FEditorValidatorBlueprintCallRuleBuilder& Message(const FString& In)
	{
		Rule.RuleText = FText::FromString(In);
		return *this;
	}

	FEditorValidatorBlueprintCallRuleBuilder& When(TFunction<bool(const UBlueprint*)> Condition)
	{
		Rule.Applies = MoveTemp(Condition);
		return *this;
	}

	FEditorValidatorBlueprintCallRuleBuilder& Suggest(const FString& In)
	{
		Suggestions.Add(FEditorValidatorBlueprintSuggestion(FText::FromString(In)));
		return *this;
	}

	FEditorValidatorBlueprintCallRuleBuilder& SuggestCopy(const FString& Label, const FString& CopyValue)
	{
		Suggestions.Add(FEditorValidatorBlueprintSuggestion::MakeCopy(Label, CopyValue));
		return *this;
	}

	FEditorValidatorBlueprintCallRuleBuilder& SuggestAction(const FString& Label, TFunction<void()> Action)
	{
		Suggestions.Add(FEditorValidatorBlueprintSuggestion::MakeAction(Label, MoveTemp(Action)));
		return *this;
	}

	FEditorValidatorBlueprintCallRuleBuilder& Info()
	{
		Rule.GetSeverity = [](const UBlueprint*) { return EMessageSeverity::Info; };
		return *this;
	}

	FEditorValidatorBlueprintCallRuleBuilder& PerformanceWarning()
	{
		Rule.GetSeverity = [](const UBlueprint*)
			{
				return EMessageSeverity::Warning;
			};

		const FText ExistingMessage = Rule.RuleText;

		Rule.RuleText = FText::Format(
			NSLOCTEXT(
				"EditorValidator",
				"PerformanceWarning",
				"[Performance] {0}"),
			ExistingMessage
		);

		return *this;
	}

	FEditorValidatorBlueprintCallRuleBuilder& Warning()
	{
		Rule.GetSeverity = [](const UBlueprint*) { return EMessageSeverity::Warning; };
		return *this;
	}

	FEditorValidatorBlueprintCallRuleBuilder& Error()
	{
		Rule.GetSeverity = [](const UBlueprint*) { return EMessageSeverity::Error; };
		return *this;
	}

	FEditorValidatorBlueprintCallRuleBuilder& CustomSeverity(TFunction<EMessageSeverity::Type(const UBlueprint*)> In)
	{
		Rule.GetSeverity = MoveTemp(In);
		return *this;
	}

	operator FEditorValidatorBlueprintCallRule()
	{
		Rule.BuildSuggestions =
			[LocalSuggestions = Suggestions](FName, TArray<FEditorValidatorBlueprintSuggestion>& Out)
			{
				Out = LocalSuggestions;
			};

		return Rule;
	}
};

/** Base validator for restricting Blueprint implementations and function calls. */
UCLASS(Abstract)
class UE_API UEditorValidator_BlueprintRestriction : public UEditorValidatorBase
{
	GENERATED_BODY()

protected:
	virtual bool AppliesTo(UObject* Asset) const PURE_VIRTUAL(UEditorValidator_BlueprintRestriction::AppliesTo, return false;);

	virtual void BuildNodeRules(TArray<FEditorValidatorBlueprintNodeRule>& OutRules) const {}
	virtual void BuildCallRules(TArray<FEditorValidatorBlueprintCallRule>& OutRules) const {}

	FEditorValidatorBlueprintNodeRuleBuilder FORBID_OVERRIDE(const FName FunctionName) const;
	FEditorValidatorBlueprintCallRuleBuilder FORBID_CALL(UClass* OwnerClass, const FName FunctionName) const;

	void ValidateBlueprint(UBlueprint* Blueprint, FDataValidationContext& Context, bool& bAnyViolation) const;

	static bool IsOverrideOf(UEdGraphNode* Node, const FName FunctionName);
	static bool IsGhostEvent(const UK2Node_Event* EventNode);

	static bool TryGetCallFunctionInfo(
		UEdGraphNode* Node,
		UClass*& OutOwnerClass,
		FName& OutNormalized,
		FString& OutWhatUsed);

	static void AddViolationMessage(
		FDataValidationContext& Context,
		UBlueprint* Blueprint,
		UEdGraph* Graph,
		UEdGraphNode* Node,
		EMessageSeverity::Type Severity,
		const FText& RuleText,
		const FString& WhatUsed,
		const TArray<FEditorValidatorBlueprintSuggestion>& Suggestions);

public:
	virtual bool CanValidateAsset_Implementation(
		const FAssetData& InAssetData,
		UObject* InObject,
		FDataValidationContext& InContext) const override;

	virtual EDataValidationResult ValidateLoadedAsset_Implementation(
		const FAssetData& InAssetData,
		UObject* InAsset,
		FDataValidationContext& Context) override;
};



/** Developer settings for configuring Blueprint validation rules. */
UCLASS(Config = Editor, defaultconfig, DisplayName = "Editor Blueprint Validation")
class UE_API UEditorValidatorBlueprintValidationSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

};

/**
 * Settings-backed Blueprint validator.
 *
 * Intentionally not exported for extension; project-specific rules should derive from
 * UEditorValidator_BlueprintRestriction instead.
 */
UCLASS()
class UEditorValidator_BlueprintRestriction_SettingBased : public UEditorValidator_BlueprintRestriction
{
	GENERATED_BODY()

protected:
	virtual bool AppliesTo(UObject* Asset) const override
	{
		return Asset && Asset->IsA<UBlueprint>();
	}

	virtual void BuildNodeRules(TArray<FEditorValidatorBlueprintNodeRule>& OutRules) const override;
};

#undef UE_API