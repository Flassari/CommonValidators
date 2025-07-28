#include "EditorValidator_PureNode.h"

#include "Engine/Blueprint.h"
#include "Misc/DataValidation.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_Variable.h"
#include "CommonValidatorsStatics.h"
#include "CommonValidatorsDeveloperSettings.h"
#include "K2Node.h"

namespace UE::Internal::PureNodeValidatorHelpers
{
	bool IsHarmlessPureNode(UK2Node_CallFunction* CallNode)
	{
		if (!CallNode) return false;

		UFunction* Func = CallNode->GetTargetFunction();
		if (!Func)
		{
			return false;
		}

		if (Func->HasMetaData(TEXT("NativeBreakFunc")) || Func->HasMetaData(TEXT("NativeMakeFunc")))
		{
			return true;
		}

		UClass* OwnerClass = Func->GetOuterUClass();
		if (!OwnerClass)
		{
			return false;
		}
		
		const FString OwnerName = OwnerClass->GetName();

		if (OwnerName.Contains(TEXT("KismetMathLibrary")) ||
			OwnerName.Contains(TEXT("KismetSystemLibrary")) ||
			OwnerName.Contains(TEXT("KismetTextLibrary")) ||
			OwnerName.Contains(TEXT("KismetStringTableLibrary")) ||
			OwnerName.Contains(TEXT("KismetRenderingLibrary")) ||
			OwnerName.Contains(TEXT("KismetMaterialLibrary")) ||
			OwnerName.Contains(TEXT("KismetInternationalizationLibrary")) ||
			OwnerName.Contains(TEXT("KismetInputLibrary")) ||
			OwnerName.Contains(TEXT("KismetGuidLibrary")) ||
			OwnerName.Contains(TEXT("KismetArrayLibrary")) ||
			OwnerName.Contains(TEXT("GameplayStatics")) ||
			OwnerName.Contains(TEXT("DataTableFunctionLibrary")) ||
			OwnerName.Contains(TEXT("BlueprintSetLibrary")) ||
			OwnerName.Contains(TEXT("BlueprintPlatformLibrary")) ||
			OwnerName.Contains(TEXT("BlueprintPathsLibrary")) ||
			OwnerName.Contains(TEXT("BlueprintMapLibrary")) ||
			OwnerName.Contains(TEXT("BlueprintInstancedStructLibrary")) ||
			OwnerName.Contains(TEXT("KismetNodeHelperLibrary")))
		{
			return true;
		}

		//TODO: Allow users to expand the list above? --KaosSpectrum
		//Maybe find a better way (though i can't think of one...)
		

		return false;
	}
	
    // Finds every event/function entry (no incoming exec) in the graph.
    static void CollectExecEntries(UEdGraph* Graph, TSet<UEdGraphNode*>& OutEntries)
    {
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (Node->FindPin(UEdGraphSchema_K2::PN_Execute, EGPD_Input) == nullptr)
            {
                OutEntries.Add(Node);
            }
        }
    }

    // Marks all nodes reachable via exec pins from those entries.
    static void CollectReachableExecNodes(UEdGraph* Graph, TSet<UEdGraphNode*>& OutReachable)
    {
        TSet<UEdGraphNode*> Visited;
        TArray<UEdGraphNode*> Queue;

        CollectExecEntries(Graph, Visited);
        for (UEdGraphNode* Entry : Visited)
        {
            Queue.Add(Entry);
        }

        while (Queue.Num() > 0)
        {
            UEdGraphNode* Current = Queue.Pop();
            OutReachable.Add(Current);

            for (UEdGraphPin* Pin : Current->Pins)
            {
                if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && Pin->Direction == EGPD_Output)
                {
                    for (UEdGraphPin* Link : Pin->LinkedTo)
                    {
                        UEdGraphNode* Next = Link->GetOwningNode();
                        if (!Visited.Contains(Next))
                        {
                            Visited.Add(Next);
                            Queue.Add(Next);
                        }
                    }
                }
            }
        }
    }

    // Walks the data‑pin graph until the first connected exec‑input is found.
    static UEdGraphNode* FindFirstImpureSink(UEdGraphNode* StartNode)
    {
        TSet<UEdGraphNode*> Visited;
        TArray<UEdGraphNode*> Queue = { StartNode };

        while (Queue.Num() > 0)
        {
            UEdGraphNode* Current = Queue.Pop();
            if (Visited.Contains(Current))
            {
                continue;
            }
            Visited.Add(Current);

            if (UEdGraphPin* ExecIn = Current->FindPin(UEdGraphSchema_K2::PN_Execute, EGPD_Input))
            {
                if (ExecIn->LinkedTo.Num() > 0)
                {
                    return Current;
                }
            }

            for (UEdGraphPin* Pin : Current->Pins)
            {
                if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
                {
                    for (UEdGraphPin* Link : Pin->LinkedTo)
                    {
                        Queue.Add(Link->GetOwningNode());
                    }
                    for (UEdGraphPin* Sub : Pin->SubPins)
                    {
                        for (UEdGraphPin* Link : Sub->LinkedTo)
                        {
                            Queue.Add(Link->GetOwningNode());
                        }
                    }
                }
            }
        }

        return nullptr;
    }

    bool WillPureNodeFireMultipleTimes(UK2Node* PureFunctionNode, UEdGraph* Graph)
    {
        TSet<UEdGraphNode*> Reachable;
        CollectReachableExecNodes(Graph, Reachable);

        TSet<UEdGraphNode*> DataConsumers;
        for (UEdGraphPin* Pin : PureFunctionNode->Pins)
        {
            if (Pin->Direction != EGPD_Output || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
            {
                continue;
            }

            for (UEdGraphPin* Link : Pin->LinkedTo)
            {
                DataConsumers.Add(Link->GetOwningNode());
            }
            for (UEdGraphPin* Sub : Pin->SubPins)
            {
                for (UEdGraphPin* Link : Sub->LinkedTo)
                {
                    DataConsumers.Add(Link->GetOwningNode());
                }
            }
        }

        TSet<UEdGraphNode*> ExecSinks;
        for (UEdGraphNode* Consumer : DataConsumers)
        {
            UEdGraphNode* Sink = FindFirstImpureSink(Consumer);
            if (Sink != nullptr && Reachable.Contains(Sink))
            {
                ExecSinks.Add(Sink);
                if (ExecSinks.Num() > 1)
                {
                    return true;
                }
            }
        }

        return false;
    }
} // namespace UE::Internal::PureNodeValidatorHelpers


bool UEditorValidator_PureNode::CanValidateAsset_Implementation(
    const FAssetData& InAssetData,
    UObject* InObject,
    FDataValidationContext& InContext) const
{
	bool bIsValidatorEnabled = GetDefault<UCommonValidatorsDeveloperSettings>()->bEnablePureNodeMultiExecValidator;
    return bIsValidatorEnabled && (InObject != nullptr) && InObject->IsA<UBlueprint>();
}


EDataValidationResult UEditorValidator_PureNode::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
    UBlueprint* Blueprint = Cast<UBlueprint>(InAsset);
    if (Blueprint == nullptr)
    {
        return EDataValidationResult::NotValidated;
    }
	bool bFoundBadNode = false;
	bool bShouldError = GetDefault<UCommonValidatorsDeveloperSettings>()->bErrorOnPureNodeMultiExec;

    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (Node->IsA<UK2Node_BreakStruct>() || Node->IsA<UK2Node_Variable>())
            {
                continue;
            }

            UK2Node_CallFunction* CallNode = Cast<UK2Node_CallFunction>(Node);
            if (CallNode == nullptr)
            {
                continue;
            }

            if (UFunction* TargetFunc = CallNode->GetTargetFunction())
            {
                if (TargetFunc->HasMetaData(TEXT("NativeBreakFunc")) ||
                    TargetFunc->HasMetaData(TEXT("NativeMakeFunc")))
                {
                    continue;
                }
            }

            if (!CallNode->IsNodePure() || UE::Internal::PureNodeValidatorHelpers::IsHarmlessPureNode(CallNode))
            {
                continue;
            }

            if (UE::Internal::PureNodeValidatorHelpers::WillPureNodeFireMultipleTimes(CallNode, Graph))
            {
                const FText Title = CallNode->GetNodeTitle(ENodeTitleType::MenuTitle);
                const FText Message = FText::Format(
                    NSLOCTEXT("PureNodeValidator", "MultiCallWarning",
                              "{0} will execute more than once. Convert to exec or avoid using across multiple exec nodes."),
                    Title
                );
                CallNode->ErrorMsg            = Message.ToString();
                CallNode->ErrorType           = bShouldError ? EMessageSeverity::Error : EMessageSeverity::Warning;
                CallNode->bHasCompilerMessage = true;

                TSharedRef<FTokenizedMessage> TokenMessage =
                    FTokenizedMessage::Create(EMessageSeverity::Warning, Message);

                TokenMessage->AddToken(
                    FActionToken::Create(
                        NSLOCTEXT("PureNodeValidator", "OpenNode", "Focus Node"),
                        NSLOCTEXT("PureNodeValidator", "OpenNodeTooltip", "Open this node in the Blueprint Editor"),
                        FOnActionTokenExecuted::CreateLambda([Blueprint, Graph, CallNode]()
                        {
                            UCommonValidatorsStatics::OpenBlueprintAndFocusNode(Blueprint, Graph, CallNode);
                        }),
                        /*bEnabled=*/false
                    )
                );

                Context.AddMessage(TokenMessage);
                Graph->NotifyNodeChanged(Node);
				bFoundBadNode = true;
            }
        }
    }

	if (bShouldError && bFoundBadNode)
	{
		return EDataValidationResult::Invalid;
	}
	
    return EDataValidationResult::Valid;
}