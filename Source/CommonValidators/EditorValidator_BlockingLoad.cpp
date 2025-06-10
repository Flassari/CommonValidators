
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

bool UEditorValidator_BlockingLoad::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	return InObject && InObject->IsA<UBlueprint>();
}

EDataValidationResult UEditorValidator_BlockingLoad::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	UBlueprint* Blueprint = Cast<UBlueprint>(InAsset);
	if (!Blueprint) return EDataValidationResult::NotValidated;

	bool bFoundBlockingLoad = false;
	EDataValidationResult DataValidationResult = EDataValidationResult::Valid;

	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (IsBlockingLoad(Node))
			{
				// Create a tokenized message with an action to open the Blueprint and focus the node
				TSharedRef<FTokenizedMessage> TokenizedMessage = FTokenizedMessage::Create(EMessageSeverity::Warning, FText::FromString(TEXT("Blocking (synchronous) loading nodes found.")));

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

				bFoundBlockingLoad = true;
			}
		}
	}

	return bFoundBlockingLoad ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

bool UEditorValidator_BlockingLoad::IsBlockingLoad(UEdGraphNode* Node)
{
	UK2Node_CallFunction* CallFunctionNode = Cast<UK2Node_CallFunction>(Node);
	static const FName LoadAssetBlockingFunctionName(TEXT("LoadAsset_Blocking"));
	static const FName LoadClassAssetBlockingFunctionName(TEXT("LoadClassAsset_Blocking"));




	if (!CallFunctionNode)
	{
		//not a function call node
		return false;
	}

	FName FunctionName = CallFunctionNode->GetFunctionName();

	//for debugging print function name

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
