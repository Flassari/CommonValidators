
#include "EditorValidator_BlockingLoad.h"

#include "Misc/DataValidation.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "Engine/MemberReference.h"
#include "CommonValidatorsStatics.h"
#include "K2Node_CallFunction.h"
#include "CommonValidatorsDeveloperSettings.h"


bool UEditorValidator_BlockingLoad::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	bool bIsValidatorEnabled = GetDefault<UCommonValidatorsDeveloperSettings>()->bEnableBlockingLoadValidator;
	return bIsValidatorEnabled && InObject && InObject->IsA<UBlueprint>();
}

EDataValidationResult UEditorValidator_BlockingLoad::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	UBlueprint* Blueprint = Cast<UBlueprint>(InAsset);
	if (!Blueprint) return EDataValidationResult::NotValidated;

	EDataValidationResult DataValidationResult = EDataValidationResult::Valid;

	TArray<UEdGraph*> AllGraphs;
	AllGraphs.Append(Blueprint->FunctionGraphs);
	AllGraphs.Append(Blueprint->UbergraphPages);
	
    for (UEdGraph* Graph : AllGraphs)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (IsBlockingLoad(Node))
			{
				bool bShouldError = GetDefault<UCommonValidatorsDeveloperSettings>()->bErrorBlockingLoad;

				// Create a tokenized message with an action to open the Blueprint and focus the node
				TSharedRef<FTokenizedMessage> TokenizedMessage = FTokenizedMessage::Create((bShouldError ? EMessageSeverity::Error : EMessageSeverity::Warning), FText::FromString(TEXT("Blocking (synchronous) loading nodes found.")));

				TokenizedMessage->AddToken(FActionToken::Create(
					FText::FromString(TEXT("Open Blueprint and Focus Node")),
					FText::FromString(TEXT("Open Blueprint and Focus Node")),
					FOnActionTokenExecuted::CreateLambda([Blueprint, Graph, Node]()
						{
							UCommonValidatorsStatics::OpenBlueprintAndFocusNode(Blueprint, Graph, Node);
						}),
					false
				));

				Context.AddMessage(TokenizedMessage);

				DataValidationResult = bShouldError ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
			}
		}
	}

	return DataValidationResult;
}

bool UEditorValidator_BlockingLoad::IsBlockingLoad(UEdGraphNode* Node)
{
	UK2Node_CallFunction* CallFunctionNode = Cast<UK2Node_CallFunction>(Node);

	if (!CallFunctionNode)
	{
		// Not a function call node
		return false;
	}

	static const FName LoadAssetBlockingFunctionName(TEXT("LoadAsset_Blocking"));
	static const FName LoadClassAssetBlockingFunctionName(TEXT("LoadClassAsset_Blocking"));
	FName FunctionName = CallFunctionNode->GetFunctionName();

	if (FunctionName == LoadAssetBlockingFunctionName)
	{
		return true;
	}

	if (FunctionName == LoadClassAssetBlockingFunctionName)
	{
		return true;
	}


	// Not a blocking (synchronous) loading function
	return false;
}
