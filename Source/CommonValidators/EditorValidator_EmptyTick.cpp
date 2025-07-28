
#include "EditorValidator_EmptyTick.h"

#include "Runtime/Launch/Resources/Version.h"
#include "Misc/DataValidation.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "CommonValidatorsStatics.h"
#include "Engine/MemberReference.h"
#include "CommonValidatorsDeveloperSettings.h"

bool UEditorValidator_EmptyTick::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6)
	// Empty ticks are automatically disabled in UE 5.6 onwards, no need to do anything for those versions
	return false;
#else
	bool bIsValidatorEnabled = GetDefault<UCommonValidatorsDeveloperSettings>()->bEnableEmptyTickNodeValidator;
	return bIsValidatorEnabled && InObject && InObject->IsA<UBlueprint>();
#endif
}

EDataValidationResult UEditorValidator_EmptyTick::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	static const FName EventTickName(TEXT("ReceiveTick"));

	UBlueprint* Blueprint = Cast<UBlueprint>(InAsset);
	if (!Blueprint) return EDataValidationResult::NotValidated;

	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
			if (EventNode && EventNode->EventReference.GetMemberName() == EventTickName)
			{
				if (IsEmptyTick(EventNode))
				{
					bool bShouldError = GetDefault<UCommonValidatorsDeveloperSettings>()->bErrorOnEmptyTickNodes;
					// add message, with two actions: one to open the blueprint and focus the node, and one to remove the empty tick node
					TSharedRef<FTokenizedMessage> TokenizedMessage = FTokenizedMessage::Create((bShouldError ? EMessageSeverity::Error : EMessageSeverity::Warning), FText::FromString(TEXT("Empty Tick nodes still produce overhead, please use or remove it. ")));
					TokenizedMessage->AddToken(FActionToken::Create(
						FText::FromString(TEXT("Open Blueprint and Focus Node")),
						FText::FromString(TEXT("Open Blueprint and Focus Node")),
						FOnActionTokenExecuted::CreateLambda([Blueprint, Graph, EventNode]()
							{
								UCommonValidatorsStatics::OpenBlueprintAndFocusNode(Blueprint, Graph, EventNode);
							}),
						false
					));

					TokenizedMessage->AddToken(FActionToken::Create(
						FText::FromString(TEXT("Remove Empty Tick Node")),
						FText::FromString(TEXT("Remove Empty Tick Node")),
						FOnActionTokenExecuted::CreateLambda([Blueprint, Graph, EventNode]()
							{
								UCommonValidatorsStatics::DeleteNodeFromBlueprint(Blueprint, Graph, EventNode);
							}),
						false
					));

					Context.AddMessage(TokenizedMessage);
					
					return bShouldError ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
				}
			}
		}
	}

	return EDataValidationResult::Valid;
}

bool UEditorValidator_EmptyTick::IsEmptyTick(UK2Node_Event* EventNode)
{
	if (EventNode->IsAutomaticallyPlacedGhostNode()) return false; // Ghost nodes aren't real nodes

	UEdGraphPin* ExecThenPin = EventNode->FindPin(UEdGraphSchema_K2::PN_Then);
	return ExecThenPin && ExecThenPin->LinkedTo.IsEmpty();
}
