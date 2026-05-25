#include "EditorValidator_BlueprintRestriction.h"
#include "CommonValidatorsStatics.h"
#include "CommonValidatorsDeveloperSettings.h"
#include "EdGraph/EdGraph.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/DataValidation.h"

FEditorValidatorBlueprintNodeRuleBuilder UEditorValidator_BlueprintRestriction::FORBID_OVERRIDE(const FName FunctionName) const
{
	FEditorValidatorBlueprintNodeRuleBuilder Builder;

	Builder.Rule.Matches = [FunctionName](UEdGraphNode* Node, const UBlueprint*)
	{
		return IsOverrideOf(Node, FunctionName);
	};

	return Builder;
}

FEditorValidatorBlueprintCallRuleBuilder UEditorValidator_BlueprintRestriction::FORBID_CALL(UClass* OwnerClass, const FName FunctionName) const
{
	FEditorValidatorBlueprintCallRuleBuilder Builder;

	Builder.Rule.OwnerRootClass = OwnerClass;
	Builder.Rule.ForbiddenFunction = FunctionName;

	return Builder;
}

bool UEditorValidator_BlueprintRestriction::IsOverrideOf(UEdGraphNode* Node, const FName FunctionName)
{
	if (UK2Node_Event* Event = Cast<UK2Node_Event>(Node))
	{
		if (IsGhostEvent(Event)) return false;
		return Event->GetFunctionName() == FunctionName;
	}

	if (UK2Node_FunctionEntry* Entry = Cast<UK2Node_FunctionEntry>(Node))
	{
		return Entry->GetGraph()->GetFName() == FunctionName;
	}

	return false;
}

bool UEditorValidator_BlueprintRestriction::IsGhostEvent(const UK2Node_Event* EventNode)
{
	if (!EventNode) return false;

	const auto State = EventNode->GetDesiredEnabledState();
	return State == ENodeEnabledState::Disabled
		|| State == ENodeEnabledState::DevelopmentOnly;
}

bool UEditorValidator_BlueprintRestriction::TryGetCallFunctionInfo(
	UEdGraphNode* Node,
	UClass*& OutOwnerClass,
	FName& OutNormalized,
	FString& OutWhatUsed)
{
	const UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node);
	if (!Call) return false;

	const UFunction* Func = Call->GetTargetFunction();
	if (!Func) return false;

	OutOwnerClass = Func->GetOuterUClass();
	OutWhatUsed = Func->GetName();

	FString Name = Func->GetName();
	Name.RemoveFromStart(TEXT("K2_"));
	Name.RemoveFromStart(TEXT("BP_"));
	OutNormalized = FName(*Name);

	return true;
}

void UEditorValidator_BlueprintRestriction::ValidateBlueprint(
	UBlueprint* Blueprint,
	FDataValidationContext& Context,
	bool& bAnyViolation) const
{
	TArray<FEditorValidatorBlueprintNodeRule> NodeRules;
	TArray<FEditorValidatorBlueprintCallRule> CallRules;

	BuildNodeRules(NodeRules);
	BuildCallRules(CallRules);

	TArray<UEdGraph*> Graphs;
	Graphs.Append(Blueprint->FunctionGraphs);
	Graphs.Append(Blueprint->UbergraphPages);

	for (UEdGraph* Graph : Graphs)
	{
		if (!Graph) continue;

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node) continue;

			for (const auto& Rule : NodeRules)
			{
				if (!Rule.Matches || !Rule.Matches(Node, Blueprint))
					continue;

				TArray<FEditorValidatorBlueprintSuggestion> Suggestions;
				if (Rule.BuildSuggestions)
				{
					Rule.BuildSuggestions(Node, Suggestions);
				}

				const EMessageSeverity::Type Severity =
					Rule.GetSeverity ? Rule.GetSeverity(Blueprint) : EMessageSeverity::Warning;

				AddViolationMessage(
					Context,
					Blueprint,
					Graph,
					Node,
					Severity,
					Rule.RuleText,
					Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString(),
					Suggestions
				);

				if (Severity == EMessageSeverity::Error || Severity == EMessageSeverity::Warning)
				{
					bAnyViolation = true;
				}
				break;
			}

			UClass* Owner = nullptr;
			FName Normalized = NAME_None;
			FString WhatUsed;

			if (!TryGetCallFunctionInfo(Node, Owner, Normalized, WhatUsed))
				continue;

			for (const auto& Rule : CallRules)
			{
				if (Rule.Applies && !Rule.Applies(Blueprint))
					continue;

				if (!Rule.OwnerRootClass || !Owner->IsChildOf(Rule.OwnerRootClass))
					continue;

				if (Rule.ForbiddenFunction != Normalized)
					continue;

				TArray<FEditorValidatorBlueprintSuggestion> Suggestions;
				if (Rule.BuildSuggestions)
				{
					Rule.BuildSuggestions(Normalized, Suggestions);
				}

				const EMessageSeverity::Type Severity =
					Rule.GetSeverity ? Rule.GetSeverity(Blueprint) : EMessageSeverity::Warning;

				AddViolationMessage(
					Context,
					Blueprint,
					Graph,
					Node,
					Severity,
					Rule.RuleText,
					WhatUsed,
					Suggestions
				);

				if (Severity == EMessageSeverity::Error || Severity == EMessageSeverity::Warning)
				{
					bAnyViolation = true;
				}
				break;
			}
		}
	}
}

void UEditorValidator_BlueprintRestriction::AddViolationMessage(
	FDataValidationContext& Context,
	UBlueprint* Blueprint,
	UEdGraph* Graph,
	UEdGraphNode* Node,
	EMessageSeverity::Type Severity,
	const FText& RuleText,
	const FString& WhatUsed,
	const TArray<FEditorValidatorBlueprintSuggestion>& Suggestions)
{
	const FString BlueprintName = Blueprint ? Blueprint->GetName() : TEXT("<UnknownBP>");
	const FString GraphName     = Graph ? Graph->GetName() : TEXT("<UnknownGraph>");
	const FString NodeTitle     = Node ? Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString() : TEXT("<UnknownNode>");

	const FString Body = FString::Printf(
		TEXT("%s\n\nYou used: %s\nLocation: %s -> %s -> %s"),
		*RuleText.ToString(),
		*WhatUsed,
		*BlueprintName,
		*GraphName,
		*NodeTitle
	);

	TSharedRef<FTokenizedMessage> Message =
		FTokenizedMessage::Create(Severity, FText::FromString(Body));

	const TWeakObjectPtr<UBlueprint> WeakBlueprint(Blueprint);
	const TWeakObjectPtr<UEdGraph> WeakGraph(Graph);
	const FGuid NodeGuid = Node ? Node->NodeGuid : FGuid();

	Message->AddToken(FActionToken::Create(
		FText::FromString(TEXT("Open Blueprint")),
		FText::FromString(TEXT("Open the Blueprint and focus this node")),
		FOnActionTokenExecuted::CreateLambda([WeakBlueprint, WeakGraph, NodeGuid]()
		{
			if (!WeakBlueprint.IsValid() || !WeakGraph.IsValid())
				return;

			for (UEdGraphNode* N : WeakGraph->Nodes)
			{
				if (N && N->NodeGuid == NodeGuid)
				{
					UCommonValidatorsStatics::OpenBlueprintAndFocusNode(
						WeakBlueprint.Get(),
						WeakGraph.Get(),
						N
					);
					break;
				}
			}
		}),
		false
	));

	for (const auto& S : Suggestions)
	{
		if (S.OnExecute)
		{
			Message->AddToken(FActionToken::Create(
				S.DisplayText,
				FText::FromString(TEXT("Execute action")),
				FOnActionTokenExecuted::CreateLambda([S]()
				{
					S.OnExecute();
				}),
				false
			));
		}
		else if (S.bAllowCopy && S.CopyText.IsSet())
		{
			const FString Copy = S.CopyText.GetValue();

			Message->AddToken(FActionToken::Create(
				S.DisplayText,
				FText::FromString(TEXT("Copy to clipboard")),
				FOnActionTokenExecuted::CreateLambda([Copy]()
				{
					FPlatformApplicationMisc::ClipboardCopy(*Copy);
				}),
				false
			));
		}
		else
		{
			Message->AddToken(FTextToken::Create(S.DisplayText));
		}
	}

	Context.AddMessage(Message);
}

bool UEditorValidator_BlueprintRestriction::CanValidateAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InObject,
	FDataValidationContext& InContext) const
{
	return Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext)
		&& AppliesTo(InObject);
}

EDataValidationResult UEditorValidator_BlueprintRestriction::ValidateLoadedAsset_Implementation(
	const FAssetData&,
	UObject* InAsset,
	FDataValidationContext& Context)
{
	bool bAnyViolation = false;

	if (UBlueprint* BP = Cast<UBlueprint>(InAsset))
	{
		ValidateBlueprint(BP, Context, bAnyViolation);
	}

	return bAnyViolation ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

static EMessageSeverity::Type ToMessageSeverity(
	EEditorValidatorBlueprintValidationRuleSeverity In)
{
	switch (In)
	{
		case EEditorValidatorBlueprintValidationRuleSeverity::Info:
			return EMessageSeverity::Info;
		case EEditorValidatorBlueprintValidationRuleSeverity::PerformanceWarning:
			return EMessageSeverity::PerformanceWarning;
		case EEditorValidatorBlueprintValidationRuleSeverity::Error:
			return EMessageSeverity::Error;
		case EEditorValidatorBlueprintValidationRuleSeverity::Warning:
		default:
			return EMessageSeverity::Warning;
	}
}

static bool MatchesBaseScope(
	const UBlueprint* BP,
	UClass* BaseClass,
	EEditorValidatorBlueprintBaseScope Scope)
{
	if (!BP || !BP->GeneratedClass || !BaseClass)
		return false;

	if (Scope == EEditorValidatorBlueprintBaseScope::BaseAndDerived)
		return BP->GeneratedClass->IsChildOf(BaseClass);

	return BP->GeneratedClass != BaseClass &&
		BP->GeneratedClass->IsChildOf(BaseClass);
}

static bool IsBlueprintImplementationFunction(const UFunction* Func)
{
	return Func &&
		Func->HasAnyFunctionFlags(FUNC_Event | FUNC_BlueprintEvent);
}

static FName ResolveBlueprintImplementationName(
	UClass* ContextClass,
	FName InputName)
{
	if (!ContextClass || InputName.IsNone())
		return InputName;

	const FString Desired = InputName.ToString();

	for (TFieldIterator<UFunction> It(ContextClass, EFieldIteratorFlags::IncludeSuper); It; ++It)
	{
		const UFunction* Func = *It;

		if (!IsBlueprintImplementationFunction(Func))
			continue;

		if (Func->GetFName() == InputName)
			return Func->GetFName();

		if (Func->GetMetaData(TEXT("DisplayName")).Equals(Desired))
			return Func->GetFName();

		if (FName::NameToDisplayString(Func->GetName(), false).Equals(Desired))
			return Func->GetFName();
	}

	return InputName;
}

void UEditorValidator_BlueprintRestriction_SettingBased::BuildNodeRules(
	TArray<FEditorValidatorBlueprintNodeRule>& OutRules) const
{
	const UCommonValidatorsDeveloperSettings* Settings =
		GetDefault<UCommonValidatorsDeveloperSettings>();

	if (!Settings)
		return;

	for (const FEditorValidatorBlueprintImplementationRule& Rule : Settings->BlueprintImplementationRules)
	{
		UClass* BaseClass = Rule.BlueprintBaseClass.LoadSynchronous();
		if (!BaseClass)
			continue;

		const FName ResolvedFunctionName =
			ResolveBlueprintImplementationName(
				BaseClass,
				Rule.FunctionToForbid
			);

		OutRules.Add(
			FORBID_OVERRIDE(ResolvedFunctionName)
				.Message(Rule.Message.ToString())
				.Suggest(Rule.Suggestion)
				.When([Rule, BaseClass](const UBlueprint* BP)
				{
					return MatchesBaseScope(
						BP,
						BaseClass,
						Rule.BaseScope
					);
				})
				.CustomSeverity([Rule](const UBlueprint*)
				{
					return ToMessageSeverity(Rule.Severity);
				})
		);
	}
}