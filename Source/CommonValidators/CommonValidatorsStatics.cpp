
// Translation Unit
#include "CommonValidatorsStatics.h"

// Unreal
#include "AssetManagerEditor/Public/AssetManagerEditorModule.h"
#include "AssetRegistry/AssetDataToken.h"
#include "BlueprintEditorModule.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ScopedTransaction.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Toolkits/AssetEditorToolkit.h"

// Project

// Local


void UCommonValidatorsStatics::OpenBlueprint(UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		return;
	}
	
	// Open the Blueprint editor (or bring it to front)
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	AssetEditorSubsystem->OpenEditorForAsset(Blueprint);
}

void UCommonValidatorsStatics::OpenBlueprintAndFocusNode(UBlueprint* Blueprint, UEdGraph* Graph, UEdGraphNode* Node)
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
}


void UCommonValidatorsStatics::DeleteNodeFromBlueprint(UBlueprint* Blueprint, UEdGraph* Graph, UEdGraphNode* Node)
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

bool UCommonValidatorsStatics::IsObjectAChildOf(const UObject* const AnyAssetReference, const TSubclassOf<UObject> ObjectClass)
{
	if (!IsValid(AnyAssetReference))
	{
		return false;
	}

	// Blueprint assets derive from UBlueprint first, so IsA won't work directly
	if (AnyAssetReference->IsA(UBlueprint::StaticClass()))
	{
		// This asset may need converted to first native class unless ObjectClass is also a BP
		const UBlueprint* const Blueprint = Cast<UBlueprint>(AnyAssetReference);
		if (!IsValid(Blueprint))
		{
			return false;
		}
		
		const UClass* const ParentClass = FBlueprintEditorUtils::FindFirstNativeClass(Blueprint->ParentClass);
		if (ParentClass->IsChildOf(ObjectClass))
		{
			return true;
		}
	}

	// Comparing actual classes
	// For non-BPs, this is correct.
	// For BPs, this works to catch ObjectClass being a child of UBlueprint, such as UAnimBlueprint.
	return AnyAssetReference->IsA(ObjectClass);
}

bool UCommonValidatorsStatics::IsAssetAChildOf(const FAssetData& AnyAssetReference, const TSubclassOf<UObject> ObjectClass)
{
	// We can do a compare natively for AssetData, but if it returns Blueprint, then we have more work to do
	const UClass* const AssetClass = AnyAssetReference.GetClass();
	if (!IsValid(AssetClass))
	{
		return false;
	}

	// Early out for native classes
	if (AssetClass->IsChildOf(ObjectClass))
	{
		return true;
	}

	if (AssetClass->IsChildOf(UBlueprint::StaticClass()))
	{
		// Get AssetClass CDO
		const UObject* const LoadedAsset = AnyAssetReference.GetAsset();
		const UBlueprint* const Blueprint = Cast<UBlueprint>(LoadedAsset);
		if (IsValid(Blueprint))
		{
			const UObject* GeneratedClassCDO = IsValid(Blueprint->GeneratedClass) ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr;
			return IsObjectAChildOf(GeneratedClassCDO, ObjectClass);
		}
	}

	return false;
}

TSharedRef<FTokenizedMessage> UCommonValidatorsStatics::CreateLinkedMessage(const FAssetData& InAssetData, const FText& Text,
	EMessageSeverity::Type Severity)
{
	// Blank Message
	TSharedRef<FTokenizedMessage> TokenizedMessage = FTokenizedMessage::Create(Severity,FText());

	TokenizedMessage->AddToken(FAssetDataToken::Create(InAssetData));

	if(!Text.IsEmpty())
	{
		TokenizedMessage->AddToken( FTextToken::Create(Text) );
	}

	return TokenizedMessage;
}

FAssetIdentifier UCommonValidatorsStatics::GetAssetIdentifierFromAssetData(const FAssetData& AssetData)
{
	// In the some MSVCs, RVO appears to only occur on the first instantiated return if the branches return
	// In the other path, non-RVO is used
	FAssetIdentifier ReturnIdentifier{};

	FPrimaryAssetId PrimaryAssetId = IAssetManagerEditorModule::ExtractPrimaryAssetIdFromFakeAssetData(AssetData);

	if (PrimaryAssetId.IsValid())
	{
		ReturnIdentifier = PrimaryAssetId;
	}
	else
	{
		ReturnIdentifier = AssetData.PackageName;
	}

	return ReturnIdentifier;
}
