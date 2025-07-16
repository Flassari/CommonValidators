#include "EditorValidator_PureNode.h"

#include "Engine/Blueprint.h"
#include "Misc/DataValidation.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_Variable.h"
#include "CommonValidatorsStatics.h"

#include "K2Node.h"


bool UEditorValidator_PureNode::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	return InObject && InObject->IsA<UBlueprint>();
}

EDataValidationResult UEditorValidator_PureNode::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	UBlueprint* Blueprint = Cast<UBlueprint>(InAsset);
	if (!Blueprint) return EDataValidationResult::NotValidated;

	bool bHasMultiPinPureNode = false;

	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UK2Node* PureNode = Cast<UK2Node>(Node);
			// If is a "UK2Node_BreakStruct" continue, these can't take 'show exec pins' anyhow
			if (Node->IsA(UK2Node_BreakStruct::StaticClass())) continue;
			// Questionable, but we don't want to show warnings for variable nodes, although they can take show exec pins and might have 'split pins'
			if (Node->IsA(UK2Node_Variable::StaticClass())) continue;
			// We should skip break/make nodes, for example, GameplayCueParameters.
			// This is pure but its native break/make. So let's skip them, they are safe --KaosSpectrum
			if (UK2Node_CallFunction* CallFunction = Cast<UK2Node_CallFunction>(Node))
			{
				UFunction* TargetFunc = CallFunction->GetTargetFunction();
				if (TargetFunc && (TargetFunc->HasMetaData(TEXT("NativeBreakFunc")) || TargetFunc->HasMetaData(TEXT("NativeMakeFunc"))))
				{
					continue;
				}
			}
			if (PureNode && PureNode->IsNodePure())
			{
				if (IsMultiPinPureNode(PureNode) && !IsWhitelistedPureNode(PureNode))
				{
					FText output = FText::Join(FText::FromString(" "), PureNode->GetNodeTitle(ENodeTitleType::Type::MenuTitle), FText::FromString(" - "), FText::FromString(TEXT("MultiPin Pure Nodes actually get called for each connected pin output.")));

					PureNode->ErrorMsg = output.ToString();
					PureNode->ErrorType = EMessageSeverity::Warning;
					PureNode->bHasCompilerMessage = true;
					
					// Create tokenized message with action using UCommonValidatorsStatics::OpenBlueprintAndFocusNode
					TSharedRef<FTokenizedMessage> TokenizedMessage = FTokenizedMessage::Create(EMessageSeverity::Warning, output);
					TokenizedMessage->AddToken(FActionToken::Create(
						FText::FromString(TEXT("Open Blueprint and Focus Node")),
						FText::FromString(TEXT("Open Blueprint and Focus Node")),
						FOnActionTokenExecuted::CreateLambda([Blueprint, Graph, PureNode]()
							{
								UCommonValidatorsStatics::OpenBlueprintAndFocusNode(Blueprint, Graph, PureNode);
							}),
						false
					));

					Context.AddMessage(TokenizedMessage);

					bHasMultiPinPureNode = true;
					Graph->NotifyNodeChanged(Node);
				}
			}
		}
	}
	if (bHasMultiPinPureNode)
	{
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}

bool UEditorValidator_PureNode::IsMultiPinPureNode(UK2Node* PureNode)
{
	int PinConnectionCount = 0;
	for (UEdGraphPin* Pin : PureNode->Pins)
	{
		// If we're an output pin and we have a connection in the graph - this counts.
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
