#include "EditorValidator_PureNode.h"

#include "Engine/Blueprint.h"
#include "Misc/DataValidation.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_BreakStruct.h"
#include "K2Node.h"


bool UEditorValidator_PureNode::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	return InObject && InObject->IsA<UBlueprint>();
}

EDataValidationResult UEditorValidator_PureNode::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	UBlueprint* Blueprint = Cast<UBlueprint>(InAsset);
	if (!Blueprint) return EDataValidationResult::NotValidated;

	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UK2Node* PureNode = Cast<UK2Node>(Node);
			if (PureNode && PureNode->IsNodePure())
			{
				if (IsMultiPinPureNode(PureNode) && !IsWhitelistedPureNode(PureNode))
				{
					FText output = FText::Join(FText::FromString(" "), PureNode->GetNodeTitle(ENodeTitleType::Type::MenuTitle), FText::FromString(TEXT("MultiPin Pure Nodes actually get called for each connected pin output.")));
					Context.AddError(output);
					return EDataValidationResult::Invalid;
				}
			}
		}
	}

	return EDataValidationResult::Valid;
}

bool UEditorValidator_PureNode::IsMultiPinPureNode(UK2Node* PureNode)
{
	int PinConnectionCount = 0;
	for (UEdGraphPin* Pin : PureNode->Pins)
	{
		//if we're an output pin and we have a connection in the graph - this counts.
		if (Pin->Direction == EGPD_Output && Pin->LinkedTo.Num() > 0)
		{
			PinConnectionCount++;
		}
	}
	
	return PinConnectionCount > 1;
}

bool UEditorValidator_PureNode::IsWhitelistedPureNode(UK2Node* PureNode)
{
	static const TArray<UClass*> WhitelistedTypes = {
		UK2Node_BreakStruct::StaticClass()
		// Add here any other classes that should be whitelisted
	};

	for (UClass* WhitelistedClass : WhitelistedTypes)
	{
		if (PureNode->IsA(WhitelistedClass))
		{
			return true;
		}
	}
	return false;
}
