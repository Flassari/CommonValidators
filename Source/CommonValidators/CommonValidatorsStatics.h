#include "Kismet2/KismetEditorUtilities.h"
#include "BlueprintEditorModule.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "IAssetTools.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Toolkits/AssetEditorToolkit.h"

class COMMONVALIDATORS_API UCommonValidatorsStatics : public UBlueprintFunctionLibrary
{
public:
    UFUNCTION()
    static void OpenBlueprintAndFocusNode(UBlueprint* Blueprint, UEdGraph* Graph, UEdGraphNode* Node)
    {
        if (!Blueprint || !Graph || !Node) return;

        // Open the Blueprint editor (or bring it to front)
        UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
        AssetEditorSubsystem->OpenEditorForAsset(Blueprint);

        // Get the currently opened asset editor
        IAssetEditorInstance* EditorInstance = AssetEditorSubsystem->FindEditorForAsset(Blueprint, /*bFocusIfOpen=*/false);
        if (!EditorInstance) return;

        // Get the Blueprint editor interface
        IBlueprintEditor* BlueprintEditor = static_cast<IBlueprintEditor*>(EditorInstance);
        if (!BlueprintEditor) return;

        // Open the graph tab and focus on the graph
        BlueprintEditor->OpenGraphAndBringToFront(Graph);

        // Select and zoom to the node
        BlueprintEditor->JumpToHyperlink(Node, false); // false: don't request rename
    };

    UFUNCTION()
    static void DeleteNodeFromBlueprint(UBlueprint* Blueprint, UEdGraph* Graph, UEdGraphNode* Node)
    {
        if (!Blueprint || !Graph || !Node) return;

        // Begin a transaction for undo/redo support
        const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "DeleteNode", "Delete Node"));

        // Mark Blueprint and Graph as modified
        Blueprint->Modify();
        Graph->Modify();
        Node->Modify();

        // Remove the node from the graph
        Node->DestroyNode();

        // Mark Blueprint as structurally modified (will trigger recompilation)
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    }
};